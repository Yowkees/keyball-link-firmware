// Copyright 2024 keyball-custom contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "kb_macro.h"
#include "quantum.h"
#include "eeprom.h"
#include <string.h>

// EEPROM配置: KB_SETTINGS(0x0248, 8バイト)の直後
// 400バイトのバッファ (0x0250〜0x03DF)
#define MACRO_EEPROM_BASE 0x0250

// ホールド中のキーを記録（マクロキー解放時にまとめてUP）
#define MACRO_MAX_HELD 8
static uint16_t g_held_keys[MACRO_MAX_HELD];
static uint8_t  g_held_count = 0;

void kb_macro_buffer_read(uint16_t offset, uint8_t *buf, uint8_t len) {
    if (offset >= MACRO_BUFFER_SIZE) { memset(buf, 0, len); return; }
    if ((uint16_t)offset + len > MACRO_BUFFER_SIZE) {
        len = (uint8_t)(MACRO_BUFFER_SIZE - offset);
    }
    eeprom_read_block(buf, (void *)(uintptr_t)(MACRO_EEPROM_BASE + offset), len);
}

void kb_macro_buffer_write(uint16_t offset, const uint8_t *buf, uint8_t len) {
    if (offset >= MACRO_BUFFER_SIZE) return;
    if ((uint16_t)offset + len > MACRO_BUFFER_SIZE) {
        len = (uint8_t)(MACRO_BUFFER_SIZE - offset);
    }
    eeprom_write_block(buf, (void *)(uintptr_t)(MACRO_EEPROM_BASE + offset), len);
}

void kb_macro_play(uint8_t idx) {
    uint16_t pos = 0;

    // idx番目のマクロの先頭まで読み飛ばす
    for (uint8_t m = 0; m < idx && pos < MACRO_BUFFER_SIZE; m++) {
        while (pos < MACRO_BUFFER_SIZE) {
            uint8_t b = eeprom_read_byte((uint8_t *)(uintptr_t)(MACRO_EEPROM_BASE + pos));
            pos++;
            if (b == MACRO_ACTION_END) break;
            // アクションは必ず3バイト（type + hi + lo）
            if (b == MACRO_ACTION_TAP || b == MACRO_ACTION_DOWN || b == MACRO_ACTION_DELAY) pos += 2;
        }
    }

    // バッファが初期化されていない場合（先頭が不正値）は何もしない
    if (pos >= MACRO_BUFFER_SIZE) return;
    uint8_t first = eeprom_read_byte((uint8_t *)(uintptr_t)(MACRO_EEPROM_BASE + pos));
    if (first != MACRO_ACTION_TAP && first != MACRO_ACTION_DOWN && first != MACRO_ACTION_DELAY && first != MACRO_ACTION_END) return;

    // マクロを再生
    while (pos < MACRO_BUFFER_SIZE) {
        uint8_t action = eeprom_read_byte((uint8_t *)(uintptr_t)(MACRO_EEPROM_BASE + pos));
        pos++;
        if (action == MACRO_ACTION_END) break;
        if (action != MACRO_ACTION_TAP && action != MACRO_ACTION_DOWN && action != MACRO_ACTION_DELAY) break;

        uint8_t hi = eeprom_read_byte((uint8_t *)(uintptr_t)(MACRO_EEPROM_BASE + pos)); pos++;
        uint8_t lo = eeprom_read_byte((uint8_t *)(uintptr_t)(MACRO_EEPROM_BASE + pos)); pos++;
        uint16_t val = ((uint16_t)hi << 8) | lo;

        if (action == MACRO_ACTION_TAP) {
            tap_code16(val);
        } else if (action == MACRO_ACTION_DOWN) {
            register_code16(val);
            if (g_held_count < MACRO_MAX_HELD) g_held_keys[g_held_count++] = val;
        } else if (action == MACRO_ACTION_DELAY) {
            wait_ms(val);
        }
    }
}

void kb_macro_release(void) {
    // ホールド中のキーを逆順に解放（修飾キーを最後に離すため）
    while (g_held_count > 0) {
        unregister_code16(g_held_keys[--g_held_count]);
    }
}
