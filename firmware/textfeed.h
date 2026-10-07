// Pico USB Keyboard (from Pico64 Keyboard) – text feed: types text in BT-64 macro syntax
// Copyright (c) 2026 Martin Oswald (do2mad, 1mhz.de) - SPDX-License-Identifier: MIT
#ifndef PICO64_TEXTFEED_H
#define PICO64_TEXTFEED_H

#include <stddef.h>
#include <stdbool.h>
#include "matrix.h"

#define TEXTFEED_MAX 1024

bool textfeed_start(const char *text, size_t len);  // false if busy or too long
bool textfeed_busy(void);
void textfeed_abort(void);
// Next keyboard state to show (press / release steps). Returns false when done.
bool textfeed_next(kb_state_t *out);

#endif
