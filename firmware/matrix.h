// Pico USB Keyboard (from Pico64 Keyboard) – C64 keyboard matrix and joystick emulation (core 1)
// Copyright (c) 2026 Martin Oswald (do2mad, 1mhz.de) - SPDX-License-Identifier: MIT
#ifndef PICO64_MATRIX_H
#define PICO64_MATRIX_H

#include <stdint.h>
#include <stdbool.h>

// Pin assignment (only GP0..GP22 are 5 V tolerant and free on the Pico 2 W;
// GP26..GP28 are ADC pins and NOT 5 V tolerant - never connect C64 lines there)
#define PIN_PA_BASE   0    // GP0..GP7  <-> C64 CIA1 PA0..PA7 (columns; read, and pulled low for joystick 2)
#define PIN_PB_BASE   8    // GP8..GP15  -> C64 CIA1 PB0..PB7 (rows, open drain: low or released)
#define PIN_RESTORE   16   // GP16       -> C64 RESTORE (open drain)

// Complete keyboard state: cols[n] = PB rows pressed in PA column n
typedef struct {
    uint8_t cols[8];
    bool    restore;
} kb_state_t;

// C64 matrix positions (PA column, PB row) of the modifier keys
#define KB_LSHIFT_COL 1
#define KB_LSHIFT_ROW 7
#define KB_RSHIFT_COL 6
#define KB_RSHIFT_ROW 4
#define KB_CMDR_COL   7
#define KB_CMDR_ROW   5
#define KB_CTRL_COL   7
#define KB_CTRL_ROW   2

// Joystick bits - identical to the CIA bit positions:
// port 1 = PB0..PB4, port 2 = PA0..PA4
#define JOY_UP    0x01
#define JOY_DOWN  0x02
#define JOY_LEFT  0x04
#define JOY_RIGHT 0x08
#define JOY_FIRE  0x10

void matrix_init(void);                     // set up pins, start core 1
void matrix_apply(const kb_state_t *s);     // publish a new keyboard state
void matrix_release_all(void);
void matrix_set_joystick(uint8_t port1, uint8_t port2);   // JOY_* bits, 0 = released

static inline void kb_press(kb_state_t *s, uint8_t col, uint8_t row) {
    s->cols[col & 7] |= (uint8_t)(1u << (row & 7));
}

/** Press the key with matrix code PA*8+PB (0..63). */
static inline void kb_press_code(kb_state_t *s, uint8_t code) {
    if (code < 64) kb_press(s, code >> 3, code & 7);
}

#endif
