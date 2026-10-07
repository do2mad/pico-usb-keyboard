// Pico USB Keyboard – BLE keyboard service for the app (compatible with the BT-64 BLE keyboard service v1)
// Copyright (c) 2026 Martin Oswald (do2mad, 1mhz.de) - SPDX-License-Identifier: MIT
//
// Everything here runs in the BTstack context (single threaded). Key states
// from the app go to the key scheduler (keys.c), which holds every state long
// enough for the KERNAL. Same as firmware/ble_service.c, without Bluepad32.

#include <stdio.h>
#include <string.h>

#include "btstack.h"
#include "pico/cyw43_arch.h"
#include "pico/time.h"

#include "ble_service.h"
#include "matrix.h"
#include "textfeed.h"
#include "keys.h"
#include "pc_keys.h"
#include "pico_usb_keyboard.h"            // generated from pico_usb_keyboard.gatt

#define PROTOCOL_VERSION 1
#define CAPS_FULL_MATRIX 0x01
#define CAPS_HID_KEYS    0x02   // C64B0006: USB key states (PC mode)
#define CAPS_PC_TEXT     0x04   // C64B0007: PC text with keyboard layout

#define FLAG_SHIFT   0x01
#define FLAG_CMDR    0x02
#define FLAG_CTRL    0x04
#define FLAG_RESTORE 0x08
#define KEY_NONE     0xFF

#define TEXT_FIRST 0x01
#define TEXT_LAST  0x02

#define ATT_APP_ERROR_BUSY     0x80
#define ATT_APP_ERROR_OVERFLOW 0x81


#define H_KEY    ATT_CHARACTERISTIC_C64B0002_B1E6_4A64_9C64_6B7E3F1A2D00_01_VALUE_HANDLE
#define H_TEXT   ATT_CHARACTERISTIC_C64B0003_B1E6_4A64_9C64_6B7E3F1A2D00_01_VALUE_HANDLE
#define H_INFO   ATT_CHARACTERISTIC_C64B0004_B1E6_4A64_9C64_6B7E3F1A2D00_01_VALUE_HANDLE
#define H_MATRIX ATT_CHARACTERISTIC_C64B0005_B1E6_4A64_9C64_6B7E3F1A2D00_01_VALUE_HANDLE
#define H_HID    ATT_CHARACTERISTIC_C64B0006_B1E6_4A64_9C64_6B7E3F1A2D00_01_VALUE_HANDLE
#define H_PCTEXT ATT_CHARACTERISTIC_C64B0007_B1E6_4A64_9C64_6B7E3F1A2D00_01_VALUE_HANDLE

// ---------------------------------------------------------------------------
// Advertising: flags, name, 128-bit service UUID (same UUID as the BT-64)

static const uint8_t adv_data[] = {
    2,  BLUETOOTH_DATA_TYPE_FLAGS, 0x06,
    // C64B0001-B1E6-4A64-9C64-6B7E3F1A2D00, little endian
    17, BLUETOOTH_DATA_TYPE_COMPLETE_LIST_OF_128_BIT_SERVICE_CLASS_UUIDS,
    0x00, 0x2D, 0x1A, 0x3F, 0x7E, 0x6B, 0x64, 0x9C, 0x64, 0x4A, 0xE6, 0xB1, 0x01, 0x00, 0x4B, 0xC6,
};
static const uint8_t scan_resp[] = {
    18, BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME,
    'P','i','c','o',' ','U','S','B',' ','K','e','y','b','o','a','r','d',
};
_Static_assert(sizeof(adv_data) <= 31, "adv_data too big");

// ---------------------------------------------------------------------------
// State

static hci_con_handle_t client = HCI_CON_HANDLE_INVALID;
static btstack_packet_callback_registration_t hci_cb;
static btstack_timer_source_t led_timer;

static char     text_buf[TEXTFEED_MAX];
static uint16_t text_len;
static bool     text_collecting;

static void add_flags(kb_state_t *s, uint8_t flags) {
    if (flags & FLAG_SHIFT) kb_press(s, KB_LSHIFT_COL, KB_LSHIFT_ROW);
    if (flags & FLAG_CMDR)  kb_press(s, KB_CMDR_COL,   KB_CMDR_ROW);
    if (flags & FLAG_CTRL)  kb_press(s, KB_CTRL_COL,   KB_CTRL_ROW);
    if (flags & FLAG_RESTORE) s->restore = true;
}

static bool any_text_busy(void) { return keys_text_busy() || pc_keys_text_busy(); }

static void app_gone(void) {
    text_collecting = false;
    pc_keys_release_all();
    if (keys_text_busy()) keys_release_all();   // a text from the app is aborted
    keys_release_source(KEYSRC_APP);
}

// ---------------------------------------------------------------------------
// ATT

// Text chunks: [flags, (layout,) bytes...]  - pc = true: PC text with layout byte
static int handle_text(const uint8_t *buffer, uint16_t size, bool pc) {
    static bool    collecting_pc;
    static uint8_t layout;
    uint16_t hdr = pc ? 2 : 1;
    if (size < hdr) return ATT_ERROR_INVALID_ATTRIBUTE_VALUE_LENGTH;
    uint8_t flags = buffer[0];
    if (flags & TEXT_FIRST) { text_len = 0; text_collecting = true; collecting_pc = pc; }
    if (!text_collecting || collecting_pc != pc) return ATT_ERROR_REQUEST_NOT_SUPPORTED;
    if (pc) layout = buffer[1];
    if (text_len + (size - hdr) > TEXTFEED_MAX) { text_collecting = false; return ATT_APP_ERROR_OVERFLOW; }
    if ((flags & TEXT_LAST) && any_text_busy()) return ATT_APP_ERROR_BUSY;   // client retries

    memcpy(&text_buf[text_len], &buffer[hdr], size - hdr);
    text_len = (uint16_t)(text_len + size - hdr);

    if (flags & TEXT_LAST) {
        text_collecting = false;
        bool ok = pc ? pc_keys_type_text(text_buf, text_len, layout) : keys_type_text(text_buf, text_len);
        if (!ok) return ATT_APP_ERROR_BUSY;
        if (!pc) printf("text feed: %u chars\n", text_len);
    }
    return 0;
}

static int att_write_cb(hci_con_handle_t con, uint16_t handle, uint16_t mode,
                        uint16_t offset, uint8_t *buffer, uint16_t size) {
    if (con != client) return 0;                // only the app writes here
    if (mode != ATT_TRANSACTION_MODE_NONE || offset != 0)
        return ATT_ERROR_REQUEST_NOT_SUPPORTED;

    kb_state_t s = {0};
    switch (handle) {
    case H_KEY:
        if (size != 2) return ATT_ERROR_INVALID_ATTRIBUTE_VALUE_LENGTH;
        if (keys_debug())
            printf("%8lu ms  BLE key flags=%02x key=%02x\n",
                   (unsigned long)to_ms_since_boot(get_absolute_time()), buffer[0], buffer[1]);
        add_flags(&s, buffer[0]);
        if (buffer[1] != KEY_NONE) kb_press(&s, (buffer[1] >> 3) & 7, buffer[1] & 7);
        keys_post(KEYSRC_APP, &s);
        return 0;
    case H_MATRIX:
        if (size != 9) return ATT_ERROR_INVALID_ATTRIBUTE_VALUE_LENGTH;
        add_flags(&s, buffer[0]);
        for (int i = 0; i < 8; i++) s.cols[i] |= buffer[1 + i];
        keys_post(KEYSRC_APP, &s);
        return 0;
    case H_TEXT:
        return handle_text(buffer, size, false);
    case H_HID:
        if (size < 1 || size > 7) return ATT_ERROR_INVALID_ATTRIBUTE_VALUE_LENGTH;
        pc_keys_post(buffer, size);
        return 0;
    case H_PCTEXT:
        return handle_text(buffer, size, true);
    default:
        return 0;
    }
}

static uint16_t att_read_cb(hci_con_handle_t con, uint16_t handle, uint16_t offset,
                            uint8_t *buffer, uint16_t size) {
    (void)con;
    if (handle == H_INFO) {
        uint8_t info[3] = { PROTOCOL_VERSION, any_text_busy() ? 1 : 0,
                            CAPS_FULL_MATRIX | CAPS_HID_KEYS | CAPS_PC_TEXT };
        return att_read_callback_handle_blob(info, sizeof(info), offset, buffer, size);
    }
    return 0;
}

// ---------------------------------------------------------------------------
// Events, advertising, status LED

static void start_advertising(void) {
    bd_addr_t null_addr = {0};
    // 100 ms interval (units of 0.625 ms)
    gap_advertisements_set_params(160, 160, 0, 0, null_addr, 0x07, 0x00);
    gap_advertisements_set_data(sizeof(adv_data), (uint8_t *)adv_data);
    gap_scan_response_set_data(sizeof(scan_resp), (uint8_t *)scan_resp);
    gap_advertisements_enable(1);
}

static bool is_app_connection(hci_con_handle_t h) {
    return gap_get_connection_type(h) == GAP_CONNECTION_LE;
}

static void print_interval(const char *what, uint16_t units) {
    printf("%s %u.%02u ms\n", what, units * 125 / 100, units * 125 % 100);
}

static void packet_handler(uint8_t type, uint16_t channel, uint8_t *packet, uint16_t size) {
    (void)channel; (void)size;
    if (type != HCI_EVENT_PACKET) return;
    switch (hci_event_packet_get_type(packet)) {
    case ATT_EVENT_CONNECTED: {
        hci_con_handle_t h = att_event_connected_get_handle(packet);
        if (!is_app_connection(h)) break;
        client = h;
        printf("app connected\n");
        // Ask for a short connection interval: 15-30 ms (units of 1.25 ms), no latency,
        // 4 s timeout - within Apple's accessory guidelines. iOS otherwise often uses
        // 30 ms or more, which makes key presses feel late.
        gap_request_connection_parameter_update(client, 12, 24, 0, 400);
        break;
    }
    case ATT_EVENT_DISCONNECTED:
        if (att_event_disconnected_get_handle(packet) != client) break;
        client = HCI_CON_HANDLE_INVALID;
        printf("app disconnected - releasing its keys\n");
        app_gone();
        break;
    case HCI_EVENT_LE_META:
        switch (hci_event_le_meta_get_subevent_code(packet)) {
        case HCI_SUBEVENT_LE_CONNECTION_COMPLETE:
            if (hci_subevent_le_connection_complete_get_role(packet) == HCI_ROLE_SLAVE)
                print_interval("app connected, interval", hci_subevent_le_connection_complete_get_conn_interval(packet));
            break;
        case HCI_SUBEVENT_LE_ENHANCED_CONNECTION_COMPLETE_V1:
            if (hci_subevent_le_enhanced_connection_complete_v1_get_role(packet) == HCI_ROLE_SLAVE)
                print_interval("app connected, interval", hci_subevent_le_enhanced_connection_complete_v1_get_conn_interval(packet));
            break;
        case HCI_SUBEVENT_LE_CONNECTION_UPDATE_COMPLETE:
            if (hci_subevent_le_connection_update_complete_get_connection_handle(packet) == client)
                print_interval("app connection interval now", hci_subevent_le_connection_update_complete_get_conn_interval(packet));
            break;
        default:
            break;
        }
        break;
    case HCI_EVENT_DISCONNECTION_COMPLETE:
        if (hci_event_disconnection_complete_get_connection_handle(packet) == client) {
            client = HCI_CON_HANDLE_INVALID;
            app_gone();
        }
        // advertising stops while the app is connected - switch it on again
        if (client == HCI_CON_HANDLE_INVALID) gap_advertisements_enable(1);
        break;
    default:
        break;
    }
}

// LED: blinking while waiting for the app, on while connected
static void led_tick(btstack_timer_source_t *ts) {
    static bool on;
    on = (client != HCI_CON_HANDLE_INVALID) ? true : !on;
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, on);
    btstack_run_loop_set_timer(ts, 500);
    btstack_run_loop_add_timer(ts);
}

// ---------------------------------------------------------------------------

void ble_service_start(void) {
    l2cap_init();
    sm_init();
    att_server_init(profile_data, att_read_cb, att_write_cb);
    att_server_register_packet_handler(packet_handler);

    hci_cb.callback = &packet_handler;
    hci_add_event_handler(&hci_cb);

    start_advertising();
    bd_addr_t addr;
    gap_local_bd_addr(addr);
    printf("app service ready (\"Pico USB Keyboard\", %s)\n", bd_addr_to_str(addr));

    led_timer.process = &led_tick;
    btstack_run_loop_set_timer(&led_timer, 500);
    btstack_run_loop_add_timer(&led_timer);
}

bool ble_service_connected(void) {
    return client != HCI_CON_HANDLE_INVALID;
}
