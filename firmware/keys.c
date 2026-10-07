// Pico USB Keyboard (from Pico64 Keyboard) – key scheduler: merges all key sources and feeds the matrix
// Copyright (c) 2026 Martin Oswald (do2mad, 1mhz.de) - SPDX-License-Identifier: MIT
//
// Runs in the BTstack context (single threaded). Every source (app, Bluetooth
// keyboard, gamepad buttons) posts complete states. The updates go through one
// queue; the timer applies them one after the other and shows each one for a
// minimum time. The matrix always gets the OR of the current state of all
// sources, so you can e.g. hold SHIFT on the Bluetooth keyboard and tap a key
// in the app.

#include "keys.h"

#include <stdio.h>
#include <string.h>

#include "btstack.h"
#include "pico/time.h"

#include "textfeed.h"

// The KERNAL scans the keyboard every 1/60 s (16.7 ms).
// App: a tap must survive at least three complete scans.
#define APP_PRESS_MS    60
#define APP_RELEASE_MS  30
// Real keyboard / gamepad: the user holds the key himself, two scans are enough.
#define KBD_PRESS_MS    40
#define KBD_RELEASE_MS  20
#define TEXT_HOLD_MS    40
#define QUEUE_LEN       32

typedef struct {
    uint8_t    src;
    kb_state_t state;
} update_t;

static update_t   queue[QUEUE_LEN];
static uint8_t    q_head, q_tail;
static kb_state_t current[KEYSRC_COUNT];
static btstack_timer_source_t hold_timer;
static bool holding;
static bool debug_log = true;

static bool q_empty(void) { return q_head == q_tail; }

static bool state_empty(const kb_state_t *s) {
    for (int i = 0; i < 8; i++) if (s->cols[i]) return false;
    return !s->restore;
}

static void merged(kb_state_t *out) {
    memset(out, 0, sizeof(*out));
    for (int k = 0; k < KEYSRC_COUNT; k++) {
        for (int i = 0; i < 8; i++) out->cols[i] |= current[k].cols[i];
        out->restore |= current[k].restore;
    }
}

static void schedule(void);

static void hold_done(btstack_timer_source_t *ts) {
    (void)ts;
    holding = false;
    schedule();
}

static void hold(uint32_t ms) {
    holding = true;
    btstack_run_loop_set_timer(&hold_timer, ms);
    btstack_run_loop_add_timer(&hold_timer);
}

static void schedule(void) {
    static bool feeding;
    if (holding) return;

    kb_state_t s;
    if (textfeed_busy()) {
        if (textfeed_next(&s)) {
            feeding = true;
            matrix_apply(&s);
            hold(TEXT_HOLD_MS);
            return;
        }
    }
    if (feeding) {                       // text feed finished: back to the live keys
        feeding = false;
        merged(&s);
        matrix_apply(&s);
    }
    if (q_empty()) return;

    update_t u = queue[q_tail];
    q_tail = (uint8_t)((q_tail + 1) % QUEUE_LEN);
    current[u.src] = u.state;
    merged(&s);
    if (debug_log) {
        printf("%8lu ms  apply ", (unsigned long)to_ms_since_boot(get_absolute_time()));
        for (int i = 0; i < 8; i++) printf("%02x", s.cols[i]);
        printf("%s\n", s.restore ? " +RESTORE" : "");
    }
    matrix_apply(&s);
    bool press = !state_empty(&u.state);
    if (u.src == KEYSRC_APP) hold(press ? APP_PRESS_MS : APP_RELEASE_MS);
    else                     hold(press ? KBD_PRESS_MS : KBD_RELEASE_MS);
}

void keys_init(void) {
    hold_timer.process = &hold_done;
}

void keys_post(keysrc_t src, const kb_state_t *s) {
    if (src >= KEYSRC_COUNT || textfeed_busy()) return;   // the text feed owns the keyboard
    uint8_t next = (uint8_t)((q_head + 1) % QUEUE_LEN);
    if (next == q_tail) q_tail = (uint8_t)((q_tail + 1) % QUEUE_LEN);   // full: drop the oldest
    queue[q_head].src = (uint8_t)src;
    queue[q_head].state = *s;
    q_head = next;
    schedule();
}

void keys_release_source(keysrc_t src) {
    if (src >= KEYSRC_COUNT) return;
    // drop pending updates of this source
    uint8_t w = q_tail;
    for (uint8_t r = q_tail; r != q_head; r = (uint8_t)((r + 1) % QUEUE_LEN))
        if (queue[r].src != src) { queue[w] = queue[r]; w = (uint8_t)((w + 1) % QUEUE_LEN); }
    q_head = w;
    memset(&current[src], 0, sizeof(current[src]));
    if (!textfeed_busy()) {
        kb_state_t s;
        merged(&s);
        matrix_apply(&s);
    }
}

void keys_release_all(void) {
    textfeed_abort();
    q_head = q_tail = 0;
    memset(current, 0, sizeof(current));
    kb_state_t none = {0};
    matrix_apply(&none);
}

bool keys_type_text(const char *text, size_t len) {
    if (!textfeed_start(text, len)) return false;
    // keys still held on other sources are released while the text is typed
    q_head = q_tail = 0;
    memset(current, 0, sizeof(current));
    schedule();
    return true;
}

bool keys_text_busy(void) { return textfeed_busy(); }

void keys_toggle_debug(void) {
    debug_log = !debug_log;
    printf("key log %s\n", debug_log ? "on" : "off");
}

bool keys_debug(void) { return debug_log; }
