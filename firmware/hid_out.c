// Pico USB Keyboard – C64 key states -> USB HID keyboard reports
// Copyright (c) 2026 Martin Oswald (do2mad, 1mhz.de) - SPDX-License-Identifier: MIT
//
// Implements the matrix.h interface of Pico64 (matrix_apply ...), so the key
// scheduler (keys.c) and the text feed (textfeed.c) are used unchanged. Instead
// of driving the C64 keyboard lines, every C64 key state becomes a USB keyboard
// report.
//
// The keys are chosen for the MiSTer C64 core (C64_MiSTer, rtl/fpga64_keyboard.vhd),
// tested on a MiSTer:
//   RUN/STOP = Esc, RESTORE = F11, C= = Tab, CTRL = left Ctrl, £ = \, ← = `,
//   ↑ = F9, = = End, @ = [, * = ], : = ;, ; = ', + = =, - = -
//   digits 1-5 via the keypad, 6-0 via the main row: the core ignores keypad 6-0
//   and remaps Shift+digit for a US layout, so with SHIFT the PC key is chosen
//   that the core turns back into the wanted C64 key.

#include "matrix.h"

#include <string.h>
#include "tusb.h"

// USB HID usage IDs (keyboard page)
enum {
    U_A = 0x04, U_1 = 0x1E, U_6 = 0x23, U_7 = 0x24, U_8 = 0x25, U_9 = 0x26, U_0 = 0x27,
    U_ENTER = 0x28, U_ESC = 0x29, U_BACKSPACE = 0x2A, U_TAB = 0x2B, U_SPACE = 0x2C,
    U_MINUS = 0x2D, U_EQUAL = 0x2E, U_LBRACE = 0x2F, U_RBRACE = 0x30, U_BACKSLASH = 0x31,
    U_SEMICOLON = 0x33, U_APOSTROPHE = 0x34, U_GRAVE = 0x35, U_COMMA = 0x36, U_DOT = 0x37,
    U_SLASH = 0x38, U_F1 = 0x3A, U_F3 = 0x3C, U_F5 = 0x3E, U_F7 = 0x40, U_F9 = 0x42,
    U_F11 = 0x44, U_HOME = 0x4A, U_END = 0x4D, U_RIGHT = 0x4F, U_DOWN = 0x51,
    U_KP1 = 0x59, U_KP2 = 0x5A, U_KP3 = 0x5B, U_KP4 = 0x5C, U_KP5 = 0x5D,
    // pseudo codes for modifiers (become bits in the report)
    M_LCTRL = 0xE0, M_LSHIFT = 0xE1, M_RSHIFT = 0xE5,
};
#define L(c) (U_A + ((c) - 'A'))

// C64 matrix code (PA*8 + PB) -> HID usage
static const uint8_t matrix_usage[64] = {
    // PA0: DEL RETURN CRSR-LR F7 F1 F3 F5 CRSR-UD
    U_BACKSPACE, U_ENTER, U_RIGHT, U_F7, U_F1, U_F3, U_F5, U_DOWN,
    // PA1: 3 W A 4 Z S E LSHIFT
    U_KP3, L('W'), L('A'), U_KP4, L('Z'), L('S'), L('E'), M_LSHIFT,
    // PA2: 5 R D 6 C F T X
    U_KP5, L('R'), L('D'), U_6, L('C'), L('F'), L('T'), L('X'),
    // PA3: 7 Y G 8 B H U V
    U_7, L('Y'), L('G'), U_8, L('B'), L('H'), L('U'), L('V'),
    // PA4: 9 I J 0 M K O N
    U_9, L('I'), L('J'), U_0, L('M'), L('K'), L('O'), L('N'),
    // PA5: + P L - . : @ ,
    U_EQUAL, L('P'), L('L'), U_MINUS, U_DOT, U_SEMICOLON, U_LBRACE, U_COMMA,
    // PA6: £ * ; HOME RSHIFT = ↑ /
    U_BACKSLASH, U_RBRACE, U_APOSTROPHE, U_HOME, M_RSHIFT, U_END, U_F9, U_SLASH,
    // PA7: 1 ← CTRL 2 SPACE C= Q RUN/STOP
    U_KP1, U_GRAVE, M_LCTRL, U_KP2, U_SPACE, U_TAB, L('Q'), U_ESC,
};

// With SHIFT: C64 6 -> PC 7, 7 -> 6, 8 -> 9, 9 -> 0, 0 -> nothing (see above)
static uint8_t shifted_digit(uint8_t code) {
    switch (code) {
    case 0x13: return U_7;
    case 0x18: return U_6;
    case 0x1B: return U_9;
    case 0x20: return U_0;
    case 0x23: return 0;
    default:   return 0xFF;      // not a remapped digit
    }
}

static uint8_t report_mod;
static uint8_t report_keys[6];
static bool    dirty;

// two sources: C64 keys (keys.c -> matrix_apply) and PC keys (pc_keys.c)
static uint8_t c64_mod, c64_keys[6];
static uint8_t pc_mod, pc_keys[6];

static void merge(void) {
    uint8_t mod = c64_mod | pc_mod, keys[6] = {0};
    int n = 0;
    for (int i = 0; i < 6 && n < 6; i++) if (c64_keys[i]) keys[n++] = c64_keys[i];
    for (int i = 0; i < 6 && n < 6; i++) {
        uint8_t k = pc_keys[i];
        bool dup = false;
        for (int j = 0; j < n; j++) if (keys[j] == k) dup = true;
        if (k && !dup) keys[n++] = k;
    }
    if (mod != report_mod || memcmp(keys, report_keys, sizeof(keys)) != 0) {
        report_mod = mod;
        memcpy(report_keys, keys, sizeof(keys));
        dirty = true;
    }
}

void hid_out_set_pc(uint8_t mods, const uint8_t keys[6]) {
    pc_mod = mods;
    memcpy(pc_keys, keys, sizeof(pc_keys));
    merge();
}

static void build_report(const kb_state_t *s) {
    uint8_t mod = 0, keys[6] = {0};
    int n = 0;
    bool shift = (s->cols[KB_LSHIFT_COL] & (1u << KB_LSHIFT_ROW)) ||
                 (s->cols[KB_RSHIFT_COL] & (1u << KB_RSHIFT_ROW));

    for (uint8_t code = 0; code < 64; code++) {
        if (!(s->cols[code >> 3] & (1u << (code & 7)))) continue;
        uint8_t u = matrix_usage[code];
        if (shift) {
            uint8_t d = shifted_digit(code);
            if (d != 0xFF) u = d;
        }
        if (u == 0) continue;
        if (u >= 0xE0) { mod |= (uint8_t)(1u << (u - 0xE0)); continue; }
        if (n < 6) keys[n++] = u;
    }
    if (s->restore && n < 6) keys[n++] = U_F11;

    c64_mod = mod;
    memcpy(c64_keys, keys, sizeof(c64_keys));
    merge();
}

// --- matrix.h interface ------------------------------------------------------

void matrix_init(void) {
    report_mod = 0;
    memset(report_keys, 0, sizeof(report_keys));
    dirty = true;
}

void matrix_apply(const kb_state_t *s) {
    build_report(s);
}

void matrix_release_all(void) {
    kb_state_t none = {0};
    build_report(&none);
}

void matrix_set_joystick(uint8_t port1, uint8_t port2) {
    (void)port1; (void)port2;    // no joystick over USB (yet)
}

// --- called from the main loop -----------------------------------------------

void hid_out_init(void) {
    tusb_init();
}

void hid_out_task(void) {
    tud_task();
    if (!dirty || !tud_hid_ready()) return;
    if (tud_hid_keyboard_report(0, report_mod, report_keys)) dirty = false;
}
