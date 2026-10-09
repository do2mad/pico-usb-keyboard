// Pico USB Keyboard – PC mode: plain USB key states and PC text from the app
// Copyright (c) 2026 Martin Oswald (do2mad, 1mhz.de) - SPDX-License-Identifier: MIT
//
// Two characteristics for a normal PC / Mac / Raspberry Pi keyboard:
//
//   C64B0006  HID state  [modifiers, key1 .. key6]   (1..7 bytes, missing = 0)
//             The app sends the complete state after every change. States are
//             queued and sent in order, at least PC_STEP_MS apart, so even a
//             very short tap reaches the computer.
//
//   C64B0007  PC text    [flags, layout, UTF-8 ...]  flags bit0 first, bit1 last
//             layout 0 = US, 1 = German (Windows / Linux / Raspberry Pi / MiSTer),
//                    2 = German (Mac)
//             The text is typed with the key positions of that layout (the
//             computer must use the same keyboard layout). Newline = Enter,
//             tab = Tab, unknown characters are skipped.
//
// Runs in the BTstack context (single threaded).

#include "pc_keys.h"

#include <stdio.h>
#include <string.h>

#include "btstack.h"

#define PC_STEP_MS   10      // minimum time between two USB key states
#define TEXT_STEP_MS 12
#define QUEUE_LEN    32

// modifier bits (USB HID)
#define LCTRL  0x01
#define LSHIFT 0x02
#define LALT   0x04
#define RALT   0x40          // AltGr

void hid_out_set_pc(uint8_t mods, const uint8_t keys[6]);   // hid_out.c

typedef struct { uint8_t mods; uint8_t keys[6]; } pc_state_t;

static pc_state_t queue[QUEUE_LEN];
static uint8_t q_head, q_tail;
static btstack_timer_source_t step_timer;
static bool stepping;

// text being typed
static char     text[PC_TEXT_MAX + 1];
static uint16_t text_len, text_pos;
static bool     text_active;
static uint8_t  text_layout;
static pc_state_t pending[4];        // key strokes of the current character
static int      pending_n, pending_i;
static bool     release_next;

// ---------------------------------------------------------------------------
// Character -> key strokes

typedef struct { uint8_t mods, key; } stroke_t;

#define S(m, k) ((stroke_t){ (m), (k) })
#define NONE    ((stroke_t){ 0, 0 })
#define SPACE   0x2C

static stroke_t letter(uint32_t c, uint8_t layout) {
    bool upper = c >= 'A' && c <= 'Z';
    uint32_t l = upper ? c - 'A' + 'a' : c;
    uint8_t key = (uint8_t)(0x04 + (l - 'a'));
    if (layout == PC_LAYOUT_DE || layout == PC_LAYOUT_DE_MAC) {   // German: Y and Z swapped
        if (l == 'y') key = 0x1D;
        else if (l == 'z') key = 0x1C;
    }
    return S(upper ? LSHIFT : 0, key);
}

static stroke_t digit(uint32_t c) {
    return S(0, c == '0' ? 0x27 : (uint8_t)(0x1E + (c - '1')));
}

// returns the number of strokes (0 = not typeable); dead keys get a SPACE after them
static int char_strokes(uint32_t c, uint8_t layout, stroke_t out[2]) {
    out[1] = NONE;
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) { out[0] = letter(c, layout); return 1; }
    if (c >= '0' && c <= '9') { out[0] = digit(c); return 1; }
    switch (c) {
    case '\n': out[0] = S(0, 0x28); return 1;
    case '\t': out[0] = S(0, 0x2B); return 1;
    case ' ':  out[0] = S(0, SPACE); return 1;
    }

    if (layout == PC_LAYOUT_US_MAC) {
        // a few Option characters of the US Mac layout
        switch (c) {
        case 0x20AC: out[0] = S(LALT | LSHIFT, 0x1F); return 1;    // € Opt+Shift+2
        case 0xA3:   out[0] = S(LALT, 0x20); return 1;             // £ Opt+3
        case 0xB0:   out[0] = S(LALT | LSHIFT, 0x25); return 1;    // ° Opt+Shift+8
        }
    }
    if (layout == PC_LAYOUT_US || layout == PC_LAYOUT_US_MAC) {
        static const struct { char c; uint8_t mods, key; } us[] = {
            {'!',LSHIFT,0x1E},{'@',LSHIFT,0x1F},{'#',LSHIFT,0x20},{'$',LSHIFT,0x21},{'%',LSHIFT,0x22},
            {'^',LSHIFT,0x23},{'&',LSHIFT,0x24},{'*',LSHIFT,0x25},{'(',LSHIFT,0x26},{')',LSHIFT,0x27},
            {'-',0,0x2D},{'_',LSHIFT,0x2D},{'=',0,0x2E},{'+',LSHIFT,0x2E},{'[',0,0x2F},{'{',LSHIFT,0x2F},
            {']',0,0x30},{'}',LSHIFT,0x30},{'\\',0,0x31},{'|',LSHIFT,0x31},{';',0,0x33},{':',LSHIFT,0x33},
            {'\'',0,0x34},{'"',LSHIFT,0x34},{'`',0,0x35},{'~',LSHIFT,0x35},{',',0,0x36},{'<',LSHIFT,0x36},
            {'.',0,0x37},{'>',LSHIFT,0x37},{'/',0,0x38},{'?',LSHIFT,0x38},
        };
        for (size_t i = 0; i < sizeof(us) / sizeof(us[0]); i++)
            if ((uint32_t)(uint8_t)us[i].c == c) {
                uint8_t key = us[i].key;
                // macOS swaps the ` key (0x35) with the ISO key (0x64), see below
                if (layout == PC_LAYOUT_US_MAC && key == 0x35) key = 0x64;
                out[0] = S(us[i].mods, key);
                return 1;
            }
        return 0;
    }

    bool mac = layout == PC_LAYOUT_DE_MAC;
    uint8_t alt = mac ? LALT : RALT;        // Option (Mac) / AltGr (PC)
    stroke_t s = NONE;
    bool dead = false;
    switch (c) {
    // shifted digits
    case '!': s = S(LSHIFT, 0x1E); break;
    case '"': s = S(LSHIFT, 0x1F); break;
    case 0xA7: s = S(LSHIFT, 0x20); break;          // §
    case '$': s = S(LSHIFT, 0x21); break;
    case '%': s = S(LSHIFT, 0x22); break;
    case '&': s = S(LSHIFT, 0x23); break;
    case '/': s = S(LSHIFT, 0x24); break;
    case '(': s = S(LSHIFT, 0x25); break;
    case ')': s = S(LSHIFT, 0x26); break;
    case '=': s = S(LSHIFT, 0x27); break;
    // key right of 0: ß ? (\ on PC)
    case 0xDF: s = S(0, 0x2D); break;               // ß
    case '?': s = S(LSHIFT, 0x2D); break;
    // accent key (dead)
    case 0xB4: s = S(0, 0x2E); dead = true; break;  // ´
    case '`': s = S(LSHIFT, 0x2E); dead = true; break;
    // ü + * # '
    case 0xFC: s = S(0, 0x2F); break;               // ü
    case 0xDC: s = S(LSHIFT, 0x2F); break;          // Ü
    case '+': s = S(0, 0x30); break;
    case '*': s = S(LSHIFT, 0x30); break;
    case 0xF6: s = S(0, 0x33); break;               // ö
    case 0xD6: s = S(LSHIFT, 0x33); break;          // Ö
    case 0xE4: s = S(0, 0x34); break;               // ä
    case 0xC4: s = S(LSHIFT, 0x34); break;          // Ä
    case '#': s = S(0, 0x32); break;
    case '\'': s = S(LSHIFT, 0x32); break;
    // ^ key (dead)
    // macOS swaps the ^ key (0x35) and the < key (0x64) of ISO keyboards
    // when it does not know the keyboard type -> send them swapped on the Mac
    case '^': s = S(0, mac ? 0x64 : 0x35); dead = true; break;
    case 0xB0: s = S(LSHIFT, mac ? 0x64 : 0x35); break;          // °
    // , . - < >
    case ',': s = S(0, 0x36); break;
    case ';': s = S(LSHIFT, 0x36); break;
    case '.': s = S(0, 0x37); break;
    case ':': s = S(LSHIFT, 0x37); break;
    case '-': s = S(0, 0x38); break;
    case '_': s = S(LSHIFT, 0x38); break;
    case '<': s = S(0, mac ? 0x35 : 0x64); break;
    case '>': s = S(LSHIFT, mac ? 0x35 : 0x64); break;
    // AltGr (PC) / Option (Mac) characters
    case '@':  s = mac ? S(alt, 0x0F) : S(alt, 0x14); break;            // Opt+L / AltGr+Q
    case 0x20AC: s = S(alt, 0x08); break;                               // € Opt/AltGr+E
    case '[':  s = mac ? S(alt, 0x22) : S(alt, 0x25); break;            // Opt+5 / AltGr+8
    case ']':  s = mac ? S(alt, 0x23) : S(alt, 0x26); break;            // Opt+6 / AltGr+9
    case '{':  s = mac ? S(alt, 0x25) : S(alt, 0x24); break;            // Opt+8 / AltGr+7
    case '}':  s = mac ? S(alt, 0x26) : S(alt, 0x27); break;            // Opt+9 / AltGr+0
    case '|':  s = mac ? S(alt, 0x24) : S(alt, 0x64); break;            // Opt+7 / AltGr+<
    case '\\': s = mac ? S(alt | LSHIFT, 0x24) : S(alt, 0x2D); break;   // Opt+Shift+7 / AltGr+ß
    case '~':                                                            // Opt+N (dead) / AltGr++
        if (mac) { s = S(alt, 0x11); dead = true; } else s = S(alt, 0x30);
        break;
    default: return 0;
    }
    out[0] = s;
    if (dead) { out[1] = S(0, SPACE); return 2; }
    return 1;
}

// minimal UTF-8 decoder; returns the code point and advances *pos
static uint32_t next_char(void) {
    uint8_t b = (uint8_t)text[text_pos++];
    if (b < 0x80) return b;
    int n = (b >= 0xF0) ? 3 : (b >= 0xE0) ? 2 : (b >= 0xC0) ? 1 : 0;
    uint32_t c = b & (0x3F >> n);
    for (int i = 0; i < n && text_pos < text_len; i++) c = (c << 6) | ((uint8_t)text[text_pos++] & 0x3F);
    return c;
}

// ---------------------------------------------------------------------------
// Scheduler

static void step(btstack_timer_source_t *ts);

static void apply(const pc_state_t *s, uint32_t ms) {
    hid_out_set_pc(s->mods, s->keys);
    stepping = true;
    btstack_run_loop_set_timer(&step_timer, ms);
    btstack_run_loop_add_timer(&step_timer);
}

// next state of the text feed; false when the text is finished
static bool text_next(pc_state_t *out) {
    if (release_next) {                     // release between two strokes
        memset(out, 0, sizeof(*out));
        release_next = false;
        return true;
    }
    while (pending_i >= pending_n) {        // fetch the next character
        if (text_pos >= text_len) return false;
        uint32_t c = next_char();
        if (c == '\r') continue;
        stroke_t st[2];
        int n = char_strokes(c, text_layout, st);
        pending_n = pending_i = 0;
        for (int i = 0; i < n; i++) {
            memset(&pending[i], 0, sizeof(pending[i]));
            pending[i].mods = st[i].mods;
            pending[i].keys[0] = st[i].key;
        }
        pending_n = n;
    }
    *out = pending[pending_i++];
    release_next = true;
    return true;
}

static void step(btstack_timer_source_t *ts) {
    (void)ts;
    stepping = false;
    pc_state_t s;
    if (text_active) {
        if (text_next(&s)) { apply(&s, TEXT_STEP_MS); return; }
        text_active = false;
        memset(&s, 0, sizeof(s));
        hid_out_set_pc(0, s.keys);
        printf("PC text done\n");
    }
    if (q_head != q_tail) {
        s = queue[q_tail];
        q_tail = (uint8_t)((q_tail + 1) % QUEUE_LEN);
        apply(&s, PC_STEP_MS);
    }
}

// ---------------------------------------------------------------------------

void pc_keys_init(void) {
    step_timer.process = &step;
}

static pc_state_t app_state[PC_APPS];        // current keys of each app

// queue the combination of all apps' keys (modifiers ORed, up to 6 keys)
static void post_merged(void) {
    pc_state_t s;
    memset(&s, 0, sizeof(s));
    int n = 0;
    for (int a = 0; a < PC_APPS; a++) {
        s.mods |= app_state[a].mods;
        for (int i = 0; i < 6; i++) {
            uint8_t k = app_state[a].keys[i];
            if (!k) continue;
            bool dup = false;
            for (int j = 0; j < n; j++) if (s.keys[j] == k) dup = true;
            if (!dup && n < 6) s.keys[n++] = k;
        }
    }
    uint8_t next = (uint8_t)((q_head + 1) % QUEUE_LEN);
    if (next == q_tail) q_tail = (uint8_t)((q_tail + 1) % QUEUE_LEN);   // full: drop the oldest
    queue[q_head] = s;
    q_head = next;
    if (!stepping) step(NULL);
}

void pc_keys_post(uint8_t app, const uint8_t *data, uint16_t len) {
    if (app >= PC_APPS) return;
    pc_state_t *s = &app_state[app];
    memset(s, 0, sizeof(*s));
    if (len > 0) s->mods = data[0];
    for (uint16_t i = 1; i < len && i <= 6; i++) s->keys[i - 1] = data[i];
    if (text_active) return;                 // the text feed owns the keyboard
    post_merged();
}

void pc_keys_release_app(uint8_t app) {
    if (app >= PC_APPS) return;
    memset(&app_state[app], 0, sizeof(app_state[app]));
    if (!text_active) post_merged();
}

bool pc_keys_type_text(const char *t, uint16_t len, uint8_t layout) {
    if (text_active || len > PC_TEXT_MAX) return false;
    memcpy(text, t, len);
    text_len = len;
    text_pos = 0;
    text_layout = layout <= PC_LAYOUT_US_MAC ? layout : PC_LAYOUT_US;
    pending_n = pending_i = 0;
    release_next = false;
    q_head = q_tail = 0;
    text_active = true;
    printf("PC text: %u bytes, layout %u\n", len, text_layout);
    if (!stepping) step(NULL);
    return true;
}

bool pc_keys_text_busy(void) { return text_active; }

void pc_keys_release_all(void) {
    memset(app_state, 0, sizeof(app_state));
    text_active = false;
    q_head = q_tail = 0;
    uint8_t none[6] = {0};
    hid_out_set_pc(0, none);
}
