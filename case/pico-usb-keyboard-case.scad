// Pico USB Keyboard – Gehäuse im Retro-Heimcomputer-Stil (Keil mit Tastatur)
// für einen Raspberry Pi Pico 2 W OHNE Stiftleisten.
// Copyright (c) 2026 Martin Oswald (do2mad, 1mhz.de) – CC BY-NC-SA 4.0
//
// Zwei Teile: Oberteil (part="top") und Bodenplatte (part="bottom").
// Zusammenbau: Pico mit der Oberseite nach unten ins Oberteil legen (Führungsrippen
// richten ihn aus), Bodenplatte drauf, 4 gleiche Schrauben M1,7 x 10 (oder x 8,
// selbstschneidend) von unten durch Platte und Pico in die Dome des Oberteils.
//
// Druck: Bodenplatte flach. Oberteil mit der offenen Seite nach unten, Stützen
// "nur auf dem Druckbett" (für das schräge Dach). PLA/PETG, 0,2 mm geht,
// 0,08 mm (fein) sieht bei Logo und Tasten deutlich besser aus.
//
// Export:  openscad -D 'part="top"'    -o top.stl    pico-usb-keyboard-case.scad
//          openscad -D 'part="bottom"' -o bottom.stl pico-usb-keyboard-case.scad

part = "both";              // "top", "bottom", "both" (Vorschau)
variant = "numpad";         // rechts neben der Tastatur: "numpad" (Ziffernblock), "status" (Statusleiste) oder "both" (beides)

/* [Gehäuse] */
W  = 64;                    // Breite (x)
D  = 38;                    // Tiefe (y), vorne y=0
Hf = 10;                    // Höhe vorne
Hr = 20;                    // Höhe hinten
t  = 1.8;                   // Wandstärke
R  = 3;                     // Kantenradius
plate = 2.4;                // Dicke Bodenplatte
gap = 0.25;                 // Spiel Bodenplatte <-> Oberteil

/* [Pico 2 W] */
pcbL = 51; pcbW = 21; pcbT = 1.0;
standoff = plate + 2.6;     // Unterkante Platine (z)
holeU = [2, 49];            // Löcher: Abstand von der USB-Kante
holeV = [4.8, 16.2];        //         Abstand von der Kante mit Pin 1 (GP0)

// Position der Platine: USB-Ende rechts (+x), Pin-1-Kante hinten (+y)
pcbRight = W - t - 1.0;
pcbRear  = D/2 + pcbW/2;
function bx(u) = pcbRight - u;     // Platinen-Koordinaten -> Gehäuse
function by(v) = pcbRear - v;
pcbTop = standoff + pcbT;

/* [Löcher oben – bitte an deinem Pico nachmessen!] */
// u = Abstand von der USB-Kante, v = Abstand von der Pin-1-Kante (GP0), Mitte des Bauteils
ledU = 5.5;  ledV = 4.0;   ledD = 2.6;      // Status-LED
bootU = 12.5; bootV = 6.0; bootD = 3.4;     // BOOTSEL-Taste (mit Büroklammer drücken)

/* [USB-Öffnung] */
usbW = 10; usbH = 6.8;      // Öffnung rechts, mittig auf dem Micro-USB-Stecker
usbZ = pcbTop + 1.5;

$fn = 40;
a = atan((Hr - Hf) / D);    // Dachneigung
function h(y) = Hf + (Hr - Hf) * y / D;

// ---------------------------------------------------------------- Grundform
module wedge(w, d, hf, hr, r, x0 = 0, y0 = 0, zb = 0) {
    hull() for (x = [x0 + r, x0 + w - r], y = [y0 + r, y0 + d - r]) {
        translate([x, y, zb]) cylinder(r = r, h = 0.01);
        zt = hf + (hr - hf) * (y - y0) / d;
        translate([x, y, zt - r]) sphere(r = r);
    }
}

// Ort auf dem Dach: lokales x = Breite, y = entlang der Schräge, z = Normale
module on_roof(x, s) {
    translate([x, 0, Hf]) rotate([a, 0, 0]) translate([0, s, 0]) children();
}

module key(w = 2.8, d = 2.8, hgt = 1.1) {
    hull() {
        translate([-w/2, -d/2, -0.6]) cube([w, d, 0.6]);
        translate([-w/2 + 0.35, -d/2 + 0.35, 0]) cube([w - 0.7, d - 0.7, hgt]);
    }
}

// Rechts neben der Tastatur: Ziffernblock 3 x 3 ...
module numpad() {
    p = 3.4;
    for (r = [0:2]) for (c = [0:2])
        on_roof(50.0 + c * p, (variant == "both" ? 7.4 : 8.4) + r * p) key();
}

// ... oder Statusleiste: Rahmen um LED und BOOTSEL, darunter "PICO USB"
function roof_s(y) = y / cos(a);
module statusbar() {
    x0 = bx(bootU) - bootD/2 - 1.6; x1 = bx(ledU) + ledD/2 + 1.9;
    s0 = roof_s(min(by(bootV), by(ledV))) - 3.0;
    s1 = roof_s(max(by(bootV), by(ledV))) + 3.0;
    on_roof(0, 0) translate([0, 0, -0.5]) linear_extrude(0.5 + 0.5) difference() {
        translate([x0, s0]) offset(r = 1.2) offset(delta = -1.2) square([x1 - x0, s1 - s0]);
        translate([x0 + 0.8, s0 + 0.8]) offset(r = 0.6) offset(delta = -0.6) square([x1 - x0 - 1.6, s1 - s0 - 1.6]);
    }
    // (Schriftzug "PICO USB" entfernt - zu klein zum Drucken)
}

module keyboard() {
    p = 3.4; x0 = 5.2;
    stagger = [0, 0.9, 1.3, 0.5];
    for (r = [0:3]) for (c = [0:11])
        on_roof(x0 + stagger[r] + c * p, 8.4 + (3 - r) * p) key();
    on_roof(x0 + 2.5 * p + 3.5 * p, 4.8) key(w = 7 * p - 0.6);         // Leertaste
}

// ---------------------------------------------------------------- 1MHz.de-Logo
// Maskottchen aus Kreisen nachgebaut, Schriftzug aus dem Logo nachgezeichnet (logo-text.svg)
logoH   = 0.6;      // Höhe des Reliefs
textW   = 29;       // Breite des Schriftzugs
mascotD = 8.5;      // Größe des Maskottchens
logoX   = 4.5;      // linker Rand
logoS   = 27.5;     // Mitte auf der Dachschräge (vorne = 0)

module mascot2d() {   // Bildpunkte aus dem Logo, Höhe 160 -> mascotD
    k = mascotD / 160;
    scale([k, -k]) translate([-130, -125]) difference() {
        union() {
            translate([93, 100]) circle(r = 40);
            translate([172, 100]) circle(r = 40);
            translate([133, 160]) circle(r = 58);
        }
        // Trennlinien zwischen Augen und Schnauze
        difference() {
            union() {
                translate([93, 100]) circle(r = 40);
                translate([172, 100]) circle(r = 40);
            }
            translate([93, 100]) circle(r = 33);
            translate([172, 100]) circle(r = 33);
        }
        // Nasenlöcher
        translate([123, 200]) circle(r = 8);
        translate([143, 200]) circle(r = 8);
    }
}
module pupils2d() {
    k = mascotD / 160;
    scale([k, -k]) translate([-130, -125]) {
        translate([93, 103]) circle(r = 11);
        translate([172, 103]) circle(r = 11);
    }
}
module logo() {
    on_roof(logoX + mascotD/2, logoS) {
        translate([0, 0, -0.5]) linear_extrude(logoH + 0.5) mascot2d();
        translate([0, 0, -0.5]) linear_extrude(logoH + 0.4 + 0.5) pupils2d();
    }
    on_roof(logoX + mascotD + 1.5, logoS - 2.6)
        translate([0, 0, -0.5]) linear_extrude(logoH + 0.5)
            // fette Schrift statt der Pixelschrift aus dem Logo: deren duenne
            // Schraegstriche (z, 1) gehen beim Drucken verloren
            resize([textW, 0], auto = true)
                text("1MHz.de", size = 5, font = "Liberation Mono:style=Bold", valign = "baseline");
}

// ---------------------------------------------------------------- Dom am USB-Ende
module usb_dome(cx, cy) {
    out = cy > D/2 ? 1 : -1;                 // Richtung zur nächsten Längswand
    wallY = out > 0 ? D - t + 0.5 : t - 0.5;
    edgeY = out > 0 ? pcbRear + 0.4 : pcbRear - pcbW - 0.4;   // Platinenkante + Luft
    xs = cx - 1.5; xe = W - t + 0.5;          // bis in die Seitenwand
    // 1. Dom: auf der Platine, zur Buchse hin d = 3, nach außen d = 4.6
    translate([cx, cy, pcbTop]) cylinder(d = 3.0, h = 30);
    translate([cx, cy, pcbTop]) intersection() {
        cylinder(d = 4.6, h = 30);
        translate([-5, out > 0 ? 0 : -5, 0]) cube([10, 5, 30]);
    }
    // 2. Steg über der Platine (1,2 mm Luft für Bauteile) bis zu den Wänden
    hull() {
        translate([cx, cy, pcbTop + 1.2]) intersection() {
            cylinder(d = 3.0, h = 30);
            translate([-5, out > 0 ? 0 : -5, 0]) cube([10, 5, 30]);
        }
        translate([xs, out > 0 ? cy : wallY, pcbTop + 1.2]) cube([xe - xs, abs(wallY - cy), 30]);
    }
    // 3. außerhalb der Platine bis fast auf die Bodenplatte
    translate([xs, min(edgeY, wallY), plate + 0.3]) cube([xe - xs, abs(wallY - edgeY), 30]);
}

// ---------------------------------------------------------------- Führungsrippen
// Rippen von den Wänden bis 0,3 mm an die Platinenkante, unten 0,6 mm über der
// Bodenplatte: der Pico liegt beim Zusammenbauen (Oberteil auf dem Kopf) gerade
// und mittig, bevor die Schrauben drin sind.
guideGap = 0.3;   // Luft zur Platinenkante
guideW   = 3.2;   // Breite der Rippen
module pico_guides() {
    pl = pcbRight - pcbL; pr = pcbRight; pf = pcbRear - pcbW; pb = pcbRear;
    z0 = plate + 0.6;
    // Längsseiten (vorne und hinten), je 3 Rippen
    for (x = [pl + 8, pl + 24, pl + 40]) {
        translate([x - guideW/2, pb + guideGap, z0]) cube([guideW, D - t + 0.5 - pb - guideGap, 30]);
        translate([x - guideW/2, t - 0.5, z0]) cube([guideW, pf - guideGap - (t - 0.5), 30]);
    }
    // hinter dem Pico (Antennen-Ende), 2 Rippen, in die Schraubendome übergehend
    for (y = [by(holeV[0]), by(holeV[1])])
        translate([t - 0.5, y - guideW/2, z0]) cube([pl - guideGap - (t - 0.5), guideW, 30]);
}

// ---------------------------------------------------------------- Fase an den Rohren
// Kegel (45°) zwischen Rohr und Dachunterseite
chamfer = 2.5;
module tube_chamfer(x, y, d) {
    zin = h(y) - t / cos(a);                    // Dachunterseite über dem Rohr
    translate([x, y, zin - chamfer]) cylinder(d1 = d, d2 = d + 2 * chamfer, h = chamfer + 0.8);
}

// ---------------------------------------------------------------- Oberteil
module top() {
    difference() {
        union() {
            difference() {
                wedge(W, D, Hf, Hr, R);
                wedge(W - 2*t, D - 2*t, Hf - t, Hr - t, R - t, t, t, -1);
                translate([-1, -1, -5]) cube([W + 2, D + 2, 5]);
            }
            keyboard();
            logo();
            if (variant == "numpad" || variant == "both") numpad();
            if (variant == "status" || variant == "both") statusbar();
            intersection() {
                union() {
                    // Dome für die Schrauben (Lochpaar am Antennen-Ende)
                    for (v = holeV) translate([bx(holeU[1]), by(v), pcbTop]) cylinder(d = 5, h = 30);
                    // Dome am USB-Ende fuer M1,7-Schrauben: zur USB-Buchse hin schmal,
                    // nach außen verstärkt und mit Seiten- und Rückwand verbunden
                    for (v = holeV) usb_dome(bx(holeU[0]), by(v));
                    // Führungsrippen: Pico beim Zusammenbau mittig ausrichten
                    pico_guides();
                    // Lichtkanal LED (endet über der USB-Buchse) und Führung BOOTSEL
                    translate([bx(ledU), by(ledV), pcbTop + 3.6]) cylinder(d = ledD + 1.6, h = 30);
                    translate([bx(bootU), by(bootV), pcbTop + 3.8]) cylinder(d = bootD + 1.6, h = 30);   // 2 mm Luft zum Taster
                    // 45°-Fasen am Dach, damit die Rohre nicht abbrechen
                    tube_chamfer(bx(ledU), by(ledV), ledD + 1.6);
                    tube_chamfer(bx(bootU), by(bootV), bootD + 1.6);
                }
                wedge(W - 0.2, D - 0.2, Hf - 0.1, Hr - 0.1, R, 0.1, 0.1, -1);
            }
        }
        // Schraubenlöcher: überall M1,7 selbstschneidend (Kernloch 1,3 mm)
        for (v = holeV) translate([bx(holeU[1]), by(v), pcbTop - 1]) cylinder(d = 1.3, h = 1 + 5.5);   // endet unter dem Dach
        for (v = holeV) translate([bx(holeU[0]), by(v), pcbTop - 1]) cylinder(d = 1.3, h = 1 + 6.5);
        // LED- und BOOTSEL-Loch
        translate([bx(ledU), by(ledV), 0]) cylinder(d = ledD, h = 40);
        translate([bx(bootU), by(bootV), 0]) cylinder(d = bootD, h = 40);
        // USB-Öffnung rechts
        translate([W - t - 1, D/2 - usbW/2, usbZ - usbH/2]) cube([t + 2, usbW, usbH]);
    }
}

// ---------------------------------------------------------------- Bodenplatte
module bottom() {
    iw = W - 2*t - 2*gap; id = D - 2*t - 2*gap; ir = R - t;
    difference() {
        union() {
            hull() for (x = [t + gap + ir, t + gap + iw - ir], y = [t + gap + ir, t + gap + id - ir])
                translate([x, y, 0]) cylinder(r = ir, h = plate);
            // Abstandshalter unter den 4 Löchern
            for (u = holeU, v = holeV) translate([bx(u), by(v), 0]) cylinder(d = 4.5, h = standoff);
        }
        // Schrauben am USB-Ende (M1,7): Durchgang + Senkung für den Kopf
        for (v = holeV) {
            translate([bx(holeU[0]), by(v), -1]) cylinder(d = 1.9, h = 20);
            translate([bx(holeU[0]), by(v), -1]) cylinder(d = 3.4, h = 1 + 1.2);
        }
        // Schrauben am Antennen-Ende: Durchgang + Senkung für den Kopf
        for (v = holeV) {
            translate([bx(holeU[1]), by(v), -1]) cylinder(d = 1.9, h = 20);
            translate([bx(holeU[1]), by(v), -1]) cylinder(d = 3.4, h = 1 + 1.2);
        }
        // Mulden für Gummifüße (8 mm)
        for (x = [7, W - 7], y = [7, D - 7]) translate([x, y, -1]) cylinder(d = 8.4, h = 1 + 0.6);
    }
}

// Platine nur zur Ansicht
module pico_ghost() {
    %translate([pcbRight - pcbL, pcbRear - pcbW, standoff]) cube([pcbL, pcbW, pcbT]);
}

if (part == "top") top();
else if (part == "bottom") bottom();
else { bottom(); pico_ghost(); translate([0, 0, 0.01]) top(); }
