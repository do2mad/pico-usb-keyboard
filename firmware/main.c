// Pico USB Keyboard – the Blue-64 Keyboard app as USB keyboard (Raspberry Pi Pico 2 W)
// Copyright (c) 2026 Martin Oswald (do2mad, 1mhz.de) - SPDX-License-Identifier: MIT
//
// The Pico 2 W is a Bluetooth LE peripheral with the same service as Pico64 /
// BT-64 (so the app works unchanged) and a USB HID keyboard. C64 keys from the
// app are typed as USB keys, mapped for the MiSTer C64 core (hid_out.c).
//
// No wiring: plug the Pico into the MiSTer (or any computer) with a USB cable.
// Debug output: UART0 on GP0 (TX) / GP1 (RX), 115200 baud (optional).

#include <stdio.h>

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "btstack.h"

#include "matrix.h"
#include "keys.h"
#include "ble_service.h"
#include "pc_keys.h"

#define PICO_USB_KB_VERSION "0.3.0"

// hid_out.c (keeps tusb.h away from btstack.h - both define HID types)
void hid_out_init(void);
void hid_out_task(void);

static btstack_packet_callback_registration_t state_cb;

static void on_state(uint8_t type, uint16_t channel, uint8_t *packet, uint16_t size) {
    (void)channel; (void)size;
    if (type == HCI_EVENT_PACKET && hci_event_packet_get_type(packet) == BTSTACK_EVENT_STATE &&
        btstack_event_state_get_state(packet) == HCI_STATE_WORKING)
        ble_service_start();
}

int main(void) {
    stdio_init_all();
    hid_out_init();
    matrix_init();

    if (cyw43_arch_init()) {
        printf("cyw43 init failed\n");
        return -1;
    }
    printf("Pico USB Keyboard v" PICO_USB_KB_VERSION " starting\n");

    keys_init();
    pc_keys_init();
    state_cb.callback = &on_state;
    hci_add_event_handler(&state_cb);
    hci_power_control(HCI_POWER_ON);

    for (;;) {
        hid_out_task();                                     // USB + send changed key state
        async_context_poll(cyw43_arch_async_context());     // Bluetooth
    }
}
