# Pico USB Keyboard – the Blue-64 Keyboard app as USB keyboard (Raspberry Pi Pico 2 W)
# Build: cmake -S . -B build && cmake --build build   ->  build/pico-usb-keyboard.uf2
# keys.c / textfeed.c (key scheduler, text feed) come from Pico64 Keyboard.
cmake_minimum_required(VERSION 3.13)

set(PICO_BOARD pico2_w CACHE STRING "Board type")

include(pico_sdk_import.cmake)

project(pico_usb_keyboard C CXX ASM)
set(CMAKE_C_STANDARD 11)
pico_sdk_init()

add_executable(pico-usb-keyboard
    main.c
    hid_out.c
    pc_keys.c
    ble_service.c
    usb_descriptors.c
    keys.c
    textfeed.c
)

target_include_directories(pico-usb-keyboard PRIVATE ${CMAKE_CURRENT_LIST_DIR})
target_compile_options(pico-usb-keyboard PRIVATE -Wall -Wextra -Wno-unused-parameter)

target_link_libraries(pico-usb-keyboard
    pico_stdlib
    pico_unique_id
    pico_btstack_ble
    pico_btstack_cyw43
    pico_cyw43_arch_none
    tinyusb_device
)

pico_btstack_make_gatt_header(pico-usb-keyboard PRIVATE "${CMAKE_CURRENT_LIST_DIR}/pico_usb_keyboard.gatt")

# USB is the keyboard - debug output goes to UART0 (GP0 TX, GP1 RX, 115200)
pico_enable_stdio_usb(pico-usb-keyboard 0)
pico_enable_stdio_uart(pico-usb-keyboard 1)

pico_add_extra_outputs(pico-usb-keyboard)
