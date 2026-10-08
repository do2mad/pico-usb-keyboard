# Änderungen / Changelog

Pico USB Keyboard. Neueste Version zuerst.

## Neu (noch ohne Release) / Unreleased

### Deutsch

- **Gehäuse zum 3D-Drucken** im Ordner `case/`: Keilform im Retro-Stil mit 1MHz.de-Logo, drei Varianten (Ziffernblock, Statusleiste, beides), LED- und BOOTSEL-Öffnung, Führungsrippen für den Pico, 4 × M1,7-Schrauben. Lizenz CC BY-NC-SA 4.0.
- Eigene USB-Kennung `1209:C64B` bei pid.codes beantragt.

### English

- **3D-printable case** in the folder `case/`: retro-style wedge with the 1MHz.de logo, three variants (number pad, status bar, both), openings for LED and BOOTSEL, guide ribs for the Pico, 4 × M1.7 screws. License CC BY-NC-SA 4.0.
- Dedicated USB ID `1209:C64B` requested at pid.codes.

## v0.3.0 – 2026-10-07

### Deutsch

- Erste Veröffentlichung als eigenes Projekt (vorher Ordner `firmware-usb` in [Pico64 Keyboard](https://github.com/do2mad/pico64-keyboard), Name „Pico64 USB“). Neuer Name: **Pico USB Keyboard**, Firmware-Datei `pico-usb-keyboard.uf2`.
- **PC-Modus:** Die App zeigt eine normale PC- oder Mac-Tastatur (Layouts US, Deutsch PC, Deutsch Mac, US Mac) mit Strg, Alt, Win/Cmd, AltGr/Option, F1–F12, Pfeiltasten, bis zu 6 Tasten gleichzeitig.
- Text und Textbausteine werden im gewählten Layout getippt (Umlaute, `@ € { } [ ] \ | ~`).
- Mac: Die Tasten `^`/`<` (Deutsch) bzw. `` ` ``/`~` (US) werden vertauscht gesendet, weil macOS sie bei unbekannten Tastaturen vertauscht.
- **C64-Modus** für den C64-Core des MiSTer: C64-Tasten werden als die PC-Tasten gesendet, die der Core erwartet.

### English

- First release as a project of its own (was the folder `firmware-usb` in [Pico64 Keyboard](https://github.com/do2mad/pico64-keyboard), named "Pico64 USB"). New name: **Pico USB Keyboard**, firmware file `pico-usb-keyboard.uf2`.
- **PC mode:** the app shows a normal PC or Mac keyboard (layouts US, German PC, German Mac, US Mac) with Ctrl, Alt, Win/Cmd, AltGr/Option, F1–F12, cursor keys, up to 6 keys at once.
- Text and snippets are typed with the chosen layout (umlauts, `@ € { } [ ] \ | ~`).
- Mac: the keys `^`/`<` (German) and `` ` ``/`~` (US) are sent swapped, because macOS swaps them on unknown keyboards.
- **C64 mode** for the MiSTer C64 core: C64 keys are sent as the PC keys the core expects.
