# Änderungen / Changelog

Pico USB Keyboard. Neueste Version zuerst.

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
