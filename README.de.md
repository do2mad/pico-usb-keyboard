# Pico USB Keyboard

*English version: [README.md](README.md)*

Das Handy als **USB-Tastatur** – nur mit einem **Raspberry Pi Pico 2 W** und einem USB-Kabel.
Der Pico spricht per Bluetooth LE mit der [Blue-64-Keyboard-App](https://github.com/do2mad/blue64-keyboard-ios)
(iPhone und Android) und meldet sich am Rechner als ganz normale USB-Tastatur. Kein Löten,
keine Verkabelung, kein Treiber.

**App:** iPhone – [TestFlight-Beta](https://testflight.apple.com/join/jEn4tsP7) · Android – [APK im neuesten Release](https://github.com/do2mad/blue64-keyboard-ios/releases/latest)

Praktisch überall, wo eine große Tastatur zu viel ist: MiSTer, Raspberry Pi, Medien-PC,
Emulatoren wie VICE, kurze Eingaben unterwegs.

![Pico USB Keyboard im 3D-gedruckten Gehäuse](docs/case.jpg)

![PC-Tastatur in der App](docs/pc-keyboard.png)

## Zwei Modi

Ist die App mit einem Pico USB Keyboard verbunden, zeigt sie oben die Knöpfe **C64** und **PC**.

**PC-Modus** – eine normale PC- oder Mac-Tastatur in der App:

- Layouts: **US**, **Deutsch (PC)**, **Deutsch (Mac)**, **US (Mac)** – am Rechner dasselbe Layout einstellen
- Strg, Alt, Win / Cmd, AltGr / Option, Esc, F1–F12, Entf, Pfeiltasten, Caps Lock, bis zu 6 Tasten gleichzeitig
- Sondertasten: antippen = für die nächste Taste, zweimal antippen = eingerastet, nochmal = aus
- Bei aktivem Shift / Option / AltGr zeigen die Tasten das Zeichen, das getippt wird
- **Text und Textbausteine** werden im gewählten Layout getippt (mit Umlauten und `@ € { } [ ] \ | ~`);
  die App hat getrennte Bausteine für C64 und PC

**C64-Modus** – die C64-Tastatur der App, gemacht für den **C64-Core des MiSTer**. Die C64-Tasten
werden als die PC-Tasten gesendet, die der Core erwartet – alle C64-Tasten funktionieren, auch mit
SHIFT und C=:

| C64 | PC-Taste | C64 | PC-Taste |
|---|---|---|---|
| RUN/STOP | Esc | RESTORE | F11 |
| C= | Tab | CTRL | Strg links |
| £ | `\` | ← | `` ` `` |
| ↑ | F9 | `=` | Ende |
| `@` | `[` | `*` | `]` |
| `:` | `;` | `;` | `'` |

Ziffern 1–5 über den Ziffernblock, 6–0 über die Hauptreihe (der Core ignoriert Ziffernblock 6–0
und setzt SHIFT + Ziffer für ein US-Layout um – das berücksichtigt die Firmware).

## Loslegen

1. **Firmware aufspielen** – BOOTSEL am Pico 2 W halten, per USB anstecken,
   `pico-usb-keyboard-vX.Y.Z.uf2` aus dem [neuesten Release](https://github.com/do2mad/pico-usb-keyboard/releases/latest)
   auf das Laufwerk `RP2350` ziehen.
2. **Pico anstecken** an den Rechner / MiSTer, an dem du tippen willst.
3. **App öffnen** – sie findet **Pico USB Keyboard** von selbst. LED blinkt = wartet,
   LED an = verbunden. **Zwei Geräte** (z. B. iPhone und iPad) können gleichzeitig verbunden
   sein; ihre Tasten werden zusammengelegt wie bei zwei Tastaturen an einem Rechner.
4. Oben **C64** oder **PC** wählen, im PC-Modus dazu das Tastaturlayout.

> **Mac:** Beim ersten Anstecken öffnet macOS eventuell den „Tastatur-Assistenten“ – einfach
> schließen. Die zwei Tasten, die macOS bei unbekannten Tastaturen vertauscht, korrigiert die Firmware.

Debug-Ausgabe (optional): UART0 an GP0 (TX) / GP1 (RX), 115200 Baud.

## Gehäuse (3D-Druck)

Ein kleines Gehäuse im Retro-Stil mit 1MHz.de-Logo – Dateien in [case/](case/):

| Datei | |
|---|---|
| `top-numpad-statusbar.stl` | Oberteil mit Ziffernblock und Rahmen um LED / BOOTSEL (wie auf dem Foto) |
| `top-numpad.stl` / `top-statusbar.stl` | die beiden anderen Varianten |
| `bottom.stl` | Bodenplatte (für alle Varianten gleich) |
| `pico-usb-keyboard-case.scad` | OpenSCAD-Quelle (`variant = "numpad" / "status" / "both"`) |

- **Teile:** Raspberry Pi Pico 2 W **ohne** Stiftleisten, 4 × selbstschneidende Schrauben
  **M1,7 × 10** (× 8 geht auch), 4 Gummifüße Ø 8 mm zum Kleben (optional)
- **Drucken:** PLA oder PETG. Bodenplatte flach. Oberteil **mit der offenen Seite nach unten**,
  Stützen „nur auf dem Druckbett“ (für das schräge Dach). 0,2 mm Schicht geht, mit **0,08 mm**
  werden Logo und Tasten deutlich schöner.
- **Zusammenbau:** Pico mit der Oberseite nach unten ins Oberteil legen – die Rippen richten ihn
  aus –, Bodenplatte drauf und durch die vier Befestigungslöcher des Pico verschrauben.
- Die LED leuchtet durch das kleine Loch, BOOTSEL erreicht man mit einer Büroklammer durch das
  größere (Firmware-Update ohne Aufschrauben).

| Innen | Unterseite |
|---|---|
| ![Innen](docs/case-inside.jpg) | ![Unterseite](docs/case-bottom.jpg) |

Gehäuse ändern: OpenSCAD (der Schriftzug 1MHz.de nutzt die Schrift *Liberation Mono Bold*).

## Bluetooth-Protokoll

Gleicher Dienst wie [Pico64 Keyboard](https://github.com/do2mad/pico64-keyboard) und die
BLE-Tastaturerweiterung des BT-64 (`C64B0001-B1E6-4A64-9C64-6B7E3F1A2D00`) – die App funktioniert
mit allen dreien. Neu für den PC-Modus:

| Characteristic | Inhalt |
|---|---|
| `C64B0006` | USB-Tastenzustand `[Modifier, Taste1 … Taste6]` (USB-HID-Usage-IDs), write / write without response |
| `C64B0007` | PC-Text `[Flags, Layout, UTF-8 …]`, Flags Bit0 erster / Bit1 letzter Block, Layout 0 US, 1 DE (PC), 2 DE (Mac), 3 US (Mac) |
| `C64B0004` | Info-Byte 2 (Fähigkeiten): Bit0 volle Matrix, Bit1 USB-Tastenzustand, Bit2 PC-Text |

Die C64-Characteristics (`C64B0002` Taste, `C64B0003` Text, `C64B0005` volle Matrix) stehen im
[Pico64-Protokoll](https://github.com/do2mad/pico64-keyboard/blob/main/docs/PROTOCOL.md).

## Selbst bauen

Pico SDK 2.2.0 **mit dem Submodul `tinyusb`** (`git submodule update --init lib/tinyusb`),
ARM-GCC, CMake:

```sh
cd firmware
cmake -S . -B build && cmake --build build      # -> build/pico-usb-keyboard.uf2
```

## USB-Kennung

Die Firmware nutzt noch die Test-Kennung `1209:0001` von pid.codes. Eine eigene Kennung
(`1209:C64B`) ist bei pid.codes beantragt und wird eingetragen, sobald sie vergeben ist.

## Lizenz

Firmware und Doku: MIT – siehe [LICENSE](LICENSE) und [NOTICE](NOTICE).
Gehäuse (Ordner `case/` und die Fotos): CC BY-NC-SA 4.0 – siehe [LICENSE-CASE.md](LICENSE-CASE.md).
Von Martin Oswald (do2mad) –
[1mhz.de](https://1mhz.de).
