// Pico USB Keyboard – BTstack configuration (BLE peripheral only)
// Based on the example configuration in raspberrypi/pico-examples (pico_w/bt),
// Copyright (c) 2020 Raspberry Pi (Trading) Ltd., SPDX-License-Identifier: BSD-3-Clause.
// Adapted by Martin Oswald (do2mad, 1mhz.de).
#ifndef PICO64USB_BTSTACK_CONFIG_H
#define PICO64USB_BTSTACK_CONFIG_H

#ifndef ENABLE_BLE
#error "BLE must be enabled (link pico_btstack_ble)"
#endif

#define ENABLE_LOG_ERROR
#define ENABLE_PRINTF_HEXDUMP

#define ENABLE_LE_PERIPHERAL
#define ENABLE_LE_DATA_LENGTH_EXTENSION
#define ENABLE_LE_SECURE_CONNECTIONS
#define ENABLE_SOFTWARE_AES128
#define ENABLE_MICRO_ECC_FOR_LE_SECURE_CONNECTIONS

#define HCI_OUTGOING_PRE_BUFFER_SIZE 4
#define HCI_ACL_PAYLOAD_SIZE (255 + 4)
#define HCI_ACL_CHUNK_SIZE_ALIGNMENT 4
#define MAX_NR_HCI_CONNECTIONS 1
#define MAX_NR_L2CAP_CHANNELS 2
#define MAX_NR_L2CAP_SERVICES 2
#define MAX_NR_SM_LOOKUP_ENTRIES 3
#define MAX_NR_WHITELIST_ENTRIES 4
#define MAX_NR_LE_DEVICE_DB_ENTRIES 4
#define MAX_NR_CONTROLLER_ACL_BUFFERS 3
#define MAX_NR_CONTROLLER_SCO_PACKETS 0
#define MAX_ATT_DB_SIZE 512

#define ENABLE_HCI_CONTROLLER_TO_HOST_FLOW_CONTROL
#define HCI_HOST_ACL_PACKET_LEN (255 + 4)
#define HCI_HOST_ACL_PACKET_NUM 3
#define HCI_HOST_SCO_PACKET_LEN 0
#define HCI_HOST_SCO_PACKET_NUM 0

#define NVM_NUM_DEVICE_DB_ENTRIES 4
#define NVM_NUM_LINK_KEYS 4

#define HAVE_EMBEDDED_TIME_MS
#define HAVE_ASSERT
#define HCI_RESET_RESEND_TIMEOUT_MS 1000

#endif
