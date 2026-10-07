// Pico USB Keyboard – PC mode: plain USB key states and PC text from the app
// Copyright (c) 2026 Martin Oswald (do2mad, 1mhz.de) - SPDX-License-Identifier: MIT
#ifndef PICO64USB_PC_KEYS_H
#define PICO64USB_PC_KEYS_H

#include <stdbool.h>
#include <stdint.h>

#define PC_TEXT_MAX 1024

#define PC_LAYOUT_US      0
#define PC_LAYOUT_DE      1     // German, Windows / Linux / Raspberry Pi / MiSTer
#define PC_LAYOUT_DE_MAC  2     // German, macOS
#define PC_LAYOUT_US_MAC  3     // English (US), macOS

void pc_keys_init(void);
/** New key state from the app: [modifiers, key1..key6] (USB HID usages). */
void pc_keys_post(const uint8_t *data, uint16_t len);
/** Type UTF-8 text with the given layout. false if busy or too long. */
bool pc_keys_type_text(const char *text, uint16_t len, uint8_t layout);
bool pc_keys_text_busy(void);
void pc_keys_release_all(void);

#endif
