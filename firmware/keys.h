// Pico USB Keyboard (from Pico64 Keyboard) – key scheduler: merges all key sources and feeds the matrix
// Copyright (c) 2026 Martin Oswald (do2mad, 1mhz.de) - SPDX-License-Identifier: MIT
#ifndef PICO64_KEYS_H
#define PICO64_KEYS_H

#include <stdbool.h>
#include <stddef.h>

#include "matrix.h"

typedef enum {
    KEYSRC_APP = 0,      // iPhone / Android app (BLE service), first connection
    KEYSRC_APP2,         // second app connected at the same time
    KEYSRC_BTKBD,        // Bluetooth keyboard(s)
    KEYSRC_PAD,          // gamepad buttons mapped to keys (SPACE, RETURN, F1, RUN/STOP)
    KEYSRC_COUNT
} keysrc_t;

void keys_init(void);

/** New complete state of one source. Every change is shown for a minimum time,
 *  so the KERNAL never misses a short tap. Ignored while the text feed types. */
void keys_post(keysrc_t src, const kb_state_t *s);

/** Release everything of one source immediately (e.g. device disconnected). */
void keys_release_source(keysrc_t src);

/** Abort the text feed and release all keys of all sources. */
void keys_release_all(void);

bool keys_type_text(const char *text, size_t len);   // false if busy / too long
bool keys_text_busy(void);

void keys_toggle_debug(void);
bool keys_debug(void);

#endif
