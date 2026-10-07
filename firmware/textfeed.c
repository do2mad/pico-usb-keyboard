// Pico USB Keyboard (from Pico64 Keyboard) – text feed: types text in BT-64 macro syntax
// Copyright (c) 2026 Martin Oswald (do2mad, 1mhz.de) - SPDX-License-Identifier: MIT
//
// Syntax (compatible with the BT-64 macro feed):
//   lower-case letters  -> key unshifted  (upper case on the C64 default charset)
//   upper-case letters  -> key + SHIFT
//   digits, space and  / = ; * , @ : . - + " ! ? ] [ < > ) ( ' & % $ #  ^ (= pound)
//   ~ret~ ~del~ ~inst~ ~home~ ~clr~ ~up~ ~dn~ ~ll~ ~rr~ ~f1~..~f8~ ~stop~ ~run~
//   ~pi~ ~arup~ ~arll~ ~lsh~ ~rsh~ ~cmdr~ ~ctrl~ ~rest~
//   ~shft-psh~ ~shft-rel~ ~cmdr-psh~ ~cmdr-rel~ ~ctrl-psh~ ~ctrl-rel~   (held modifiers)
// Unknown characters are skipped.

#include "textfeed.h"
#include <string.h>

typedef struct { const char *tok; int8_t col, row; uint8_t shift; } token_t;

static const token_t tokens[] = {
    {"~ret~",  0, 1, 0}, {"~del~",  0, 0, 0}, {"~inst~", 0, 0, 1},
    {"~home~", 6, 3, 0}, {"~clr~",  6, 3, 1},
    {"~up~",   0, 7, 1}, {"~dn~",   0, 7, 0}, {"~ll~",   0, 2, 1}, {"~rr~", 0, 2, 0},
    {"~f1~",   0, 4, 0}, {"~f2~",   0, 4, 1}, {"~f3~",   0, 5, 0}, {"~f4~", 0, 5, 1},
    {"~f5~",   0, 6, 0}, {"~f6~",   0, 6, 1}, {"~f7~",   0, 3, 0}, {"~f8~", 0, 3, 1},
    {"~stop~", 7, 7, 0}, {"~run~",  7, 7, 1},
    {"~pi~",   6, 6, 1}, {"~arup~", 6, 6, 0}, {"~arll~", 7, 1, 0},
    {"~lsh~",  1, 7, 0}, {"~rsh~",  6, 4, 0}, {"~cmdr~", 7, 5, 0}, {"~ctrl~", 7, 2, 0},
    {"~rest~", -1, 0, 0},
};

// letters a..z: (column, row)
static const int8_t letters[26][2] = {
    {1,2},{3,4},{2,4},{2,2},{1,6},{2,5},{3,2},{3,5},{4,1},{4,2},{4,5},{5,2},{4,4},
    {4,7},{4,6},{5,1},{7,6},{2,1},{1,5},{2,6},{3,6},{3,7},{1,1},{2,7},{3,1},{1,4},
};
// digits 0..9
static const int8_t digits[10][2] = {
    {4,3},{7,0},{7,3},{1,0},{1,3},{2,0},{2,3},{3,0},{3,3},{4,0},
};

typedef struct { char c; int8_t col, row; uint8_t shift; } sym_t;
static const sym_t symbols[] = {
    {' ',7,4,0}, {'/',6,7,0}, {'=',6,5,0}, {';',6,2,0}, {'*',6,1,0}, {'^',6,0,0},
    {',',5,7,0}, {'@',5,6,0}, {':',5,5,0}, {'.',5,4,0}, {'-',5,3,0}, {'+',5,0,0},
    {'"',7,3,1}, {'!',7,0,1}, {'?',6,7,1}, {']',6,2,1}, {'[',5,5,1}, {'<',5,7,1},
    {'>',5,4,1}, {')',4,0,1}, {'(',3,3,1}, {'\'',3,0,1}, {'&',2,3,1}, {'%',2,0,1},
    {'$',1,3,1}, {'#',1,0,1},
};

static char     buf[TEXTFEED_MAX + 1];
static size_t   len, pos;
static bool     active;
static bool     release_next;     // after a press step comes a release step
static kb_state_t held;           // modifiers held via ~xxx-psh~

static bool starts(const char *tok) {
    size_t n = strlen(tok);
    return pos + n <= len && memcmp(&buf[pos], tok, n) == 0;
}

bool textfeed_start(const char *text, size_t n) {
    if (active || n > TEXTFEED_MAX) return false;
    memcpy(buf, text, n);
    buf[n] = 0;
    len = n;
    pos = 0;
    release_next = false;
    memset(&held, 0, sizeof(held));
    active = true;
    return true;
}

bool textfeed_busy(void) { return active; }

void textfeed_abort(void) { active = false; }

// Parses the next key at pos into *s (on top of the held modifiers).
// Returns false if nothing is left.
static bool parse_next(kb_state_t *s) {
    while (pos < len) {
        *s = held;
        char c = buf[pos];

        if (c == '~') {
            static const struct { const char *tok; int8_t col, row; bool on; } mods[] = {
                {"~shft-psh~", KB_LSHIFT_COL, KB_LSHIFT_ROW, true},
                {"~shft-rel~", KB_LSHIFT_COL, KB_LSHIFT_ROW, false},
                {"~cmdr-psh~", KB_CMDR_COL,   KB_CMDR_ROW,   true},
                {"~cmdr-rel~", KB_CMDR_COL,   KB_CMDR_ROW,   false},
                {"~ctrl-psh~", KB_CTRL_COL,   KB_CTRL_ROW,   true},
                {"~ctrl-rel~", KB_CTRL_COL,   KB_CTRL_ROW,   false},
            };
            bool found = false;
            for (size_t i = 0; i < sizeof(mods) / sizeof(mods[0]); i++) {
                if (starts(mods[i].tok)) {
                    uint8_t bit = (uint8_t)(1u << mods[i].row);
                    if (mods[i].on) held.cols[mods[i].col] |= bit;
                    else            held.cols[mods[i].col] &= (uint8_t)~bit;
                    pos += strlen(mods[i].tok);
                    found = true;
                    break;
                }
            }
            if (found) continue;
            for (size_t i = 0; i < sizeof(tokens) / sizeof(tokens[0]); i++) {
                if (starts(tokens[i].tok)) {
                    pos += strlen(tokens[i].tok);
                    if (tokens[i].col < 0) s->restore = true;
                    else kb_press(s, tokens[i].col, tokens[i].row);
                    if (tokens[i].shift) kb_press(s, KB_LSHIFT_COL, KB_LSHIFT_ROW);
                    return true;
                }
            }
            pos++;          // lone '~' - skip
            continue;
        }

        pos++;
        if (c >= 'a' && c <= 'z') {
            kb_press(s, letters[c - 'a'][0], letters[c - 'a'][1]);
            return true;
        }
        if (c >= 'A' && c <= 'Z') {
            kb_press(s, letters[c - 'A'][0], letters[c - 'A'][1]);
            kb_press(s, KB_LSHIFT_COL, KB_LSHIFT_ROW);
            return true;
        }
        if (c >= '0' && c <= '9') {
            kb_press(s, digits[c - '0'][0], digits[c - '0'][1]);
            return true;
        }
        for (size_t i = 0; i < sizeof(symbols) / sizeof(symbols[0]); i++) {
            if (symbols[i].c == c) {
                kb_press(s, symbols[i].col, symbols[i].row);
                if (symbols[i].shift) kb_press(s, KB_LSHIFT_COL, KB_LSHIFT_ROW);
                return true;
            }
        }
        // unknown character: skip
    }
    return false;
}

bool textfeed_next(kb_state_t *out) {
    if (!active) return false;
    if (release_next) {
        *out = held;                   // release the key, keep held modifiers
        release_next = false;
        return true;
    }
    if (parse_next(out)) {
        release_next = true;
        return true;
    }
    active = false;                    // done: release everything
    memset(out, 0, sizeof(*out));
    return true;
}
