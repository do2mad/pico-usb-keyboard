// Pico USB Keyboard – TinyUSB configuration: one HID boot keyboard
#ifndef PICO64USB_TUSB_CONFIG_H
#define PICO64USB_TUSB_CONFIG_H

#define CFG_TUSB_RHPORT0_MODE   OPT_MODE_DEVICE
#define CFG_TUSB_OS             OPT_OS_PICO
#define CFG_TUD_ENDPOINT0_SIZE  64

#define CFG_TUD_HID             1
#define CFG_TUD_CDC             0
#define CFG_TUD_MSC             0
#define CFG_TUD_MIDI            0
#define CFG_TUD_VENDOR          0
#define CFG_TUD_HID_EP_BUFSIZE  16

#endif
