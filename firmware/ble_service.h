// Pico USB Keyboard – BLE keyboard service for the app (compatible with the BT-64 BLE keyboard service v1)
// Copyright (c) 2026 Martin Oswald (do2mad, 1mhz.de) - SPDX-License-Identifier: MIT
#ifndef PICO64_BLE_SERVICE_H
#define PICO64_BLE_SERVICE_H

#include <stdbool.h>

/** Start GATT server + advertising. Call once the Bluetooth stack is up
 *  (BTSTACK_EVENT_STATE = HCI_STATE_WORKING). */
void ble_service_start(void);
bool ble_service_connected(void);

#endif
