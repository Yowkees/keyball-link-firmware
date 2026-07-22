// Copyright 2024 keyball-custom contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "kb_settings.h"
#include "eeprom.h"
#include <stdbool.h>
#include <string.h>

static kb_settings_t g_cache;
static bool          g_loaded = false;

kb_settings_t kb_settings_get(void) {
    if (!g_loaded) {
        eeprom_read_block(&g_cache,
            (void *)(uintptr_t)KB_SETTINGS_EEPROM_BASE,
            sizeof(kb_settings_t));
        // 0xFFFF = 未初期化EEPROM、0 = 無効値 → デフォルトへ
        if (g_cache.tapping_term == 0xFFFF || g_cache.tapping_term < 50 || g_cache.tapping_term > 1000) {
            memset(&g_cache, 0, sizeof(g_cache));
            g_cache.tapping_term = KB_SETTINGS_DEFAULT_TT;
        }
        // AML設定が未初期化(0)または範囲外なら既定値へ補正（旧FWからの移行も安全に）
        if (g_cache.aml_layer == 0 || g_cache.aml_layer > 7)         g_cache.aml_layer = 1;
        if (g_cache.aml_timeout < 100 || g_cache.aml_timeout > 5000) g_cache.aml_timeout = 650;
        if (g_cache.aml_threshold == 0 || g_cache.aml_threshold > 100) g_cache.aml_threshold = 10;
        // ジェスチャー未初期化(0xFFFF or 0)ならデフォルト割り当てへ（旧FWからの移行も安全に）
        if (g_cache.gesture[0] == 0xFFFF || g_cache.gesture[0] == 0) {
            g_cache.gesture[0] = KB_GESTURE_DEFAULT_UP;
            g_cache.gesture[1] = KB_GESTURE_DEFAULT_DOWN;
            g_cache.gesture[2] = KB_GESTURE_DEFAULT_LEFT;
            g_cache.gesture[3] = KB_GESTURE_DEFAULT_RIGHT;
        }
        // ジェスチャーのタップキーが未初期化(0xFF)なら「なし」に補正
        if (g_cache.gesture_tap == 0xFF) g_cache.gesture_tap = 0;
        g_loaded = true;
    }
    return g_cache;
}

void kb_settings_set(const kb_settings_t *s) {
    g_cache = *s;
    eeprom_write_block(s,
        (void *)(uintptr_t)KB_SETTINGS_EEPROM_BASE,
        sizeof(kb_settings_t));
}

// ── スクロールレイヤー（構造体外・EEPROM末尾に1バイト保存）────────────
static uint8_t g_scroll_layer  = 0xEE;
static bool    g_scroll_loaded = false;

uint8_t kb_scroll_layer_get(void) {
    if (!g_scroll_loaded) {
        uint8_t v = eeprom_read_byte((const uint8_t *)(uintptr_t)KB_SCROLL_LAYER_EEPROM);
        if (v <= 7)                  g_scroll_layer = v;             // 0-7 = そのレイヤー
        else if (v == KB_LAYER_NONE) g_scroll_layer = KB_LAYER_NONE; // 明示的に「なし」
        else                         g_scroll_layer = 3;            // 0xFF未初期化 → 既定レイヤー3
        g_scroll_loaded = true;
    }
    return g_scroll_layer;
}

void kb_scroll_layer_set(uint8_t v) {
    g_scroll_layer  = (v <= 7) ? v : KB_LAYER_NONE;
    g_scroll_loaded = true;
    eeprom_write_byte((uint8_t *)(uintptr_t)KB_SCROLL_LAYER_EEPROM, g_scroll_layer);
}

#ifdef GESTURE_ENABLE
// ── ジェスチャーレイヤー（同上。未初期化/0xFE は「なし」）──────────────
static uint8_t g_gesture_layer  = 0xEE;
static bool    g_gesture_loaded = false;

uint8_t kb_gesture_layer_get(void) {
    if (!g_gesture_loaded) {
        uint8_t v = eeprom_read_byte((const uint8_t *)(uintptr_t)KB_GESTURE_LAYER_EEPROM);
        g_gesture_layer = (v <= 7) ? v : KB_LAYER_NONE;  // 0-7=レイヤー / それ以外=なし
        g_gesture_loaded = true;
    }
    return g_gesture_layer;
}

void kb_gesture_layer_set(uint8_t v) {
    g_gesture_layer  = (v <= 7) ? v : KB_LAYER_NONE;
    g_gesture_loaded = true;
    eeprom_write_byte((uint8_t *)(uintptr_t)KB_GESTURE_LAYER_EEPROM, g_gesture_layer);
}

// ── ジェスチャーしきい値（横・縦、同上パターン）────────────────────────
static uint8_t g_gesture_th_h        = 0xEE;
static bool    g_gesture_th_h_loaded = false;

uint8_t kb_gesture_th_h_get(void) {
    if (!g_gesture_th_h_loaded) {
        uint8_t v = eeprom_read_byte((const uint8_t *)(uintptr_t)KB_GESTURE_TH_H_EEPROM);
        g_gesture_th_h = (v >= KB_GESTURE_TH_MIN && v <= KB_GESTURE_TH_MAX) ? v : KB_GESTURE_TH_DEFAULT;
        g_gesture_th_h_loaded = true;
    }
    return g_gesture_th_h;
}

void kb_gesture_th_h_set(uint8_t v) {
    g_gesture_th_h = (v >= KB_GESTURE_TH_MIN && v <= KB_GESTURE_TH_MAX) ? v : KB_GESTURE_TH_DEFAULT;
    g_gesture_th_h_loaded = true;
    eeprom_write_byte((uint8_t *)(uintptr_t)KB_GESTURE_TH_H_EEPROM, g_gesture_th_h);
}

static uint8_t g_gesture_th_v        = 0xEE;
static bool    g_gesture_th_v_loaded = false;

uint8_t kb_gesture_th_v_get(void) {
    if (!g_gesture_th_v_loaded) {
        uint8_t v = eeprom_read_byte((const uint8_t *)(uintptr_t)KB_GESTURE_TH_V_EEPROM);
        g_gesture_th_v = (v >= KB_GESTURE_TH_MIN && v <= KB_GESTURE_TH_MAX) ? v : KB_GESTURE_TH_DEFAULT;
        g_gesture_th_v_loaded = true;
    }
    return g_gesture_th_v;
}

void kb_gesture_th_v_set(uint8_t v) {
    g_gesture_th_v = (v >= KB_GESTURE_TH_MIN && v <= KB_GESTURE_TH_MAX) ? v : KB_GESTURE_TH_DEFAULT;
    g_gesture_th_v_loaded = true;
    eeprom_write_byte((uint8_t *)(uintptr_t)KB_GESTURE_TH_V_EEPROM, g_gesture_th_v);
}
#endif
