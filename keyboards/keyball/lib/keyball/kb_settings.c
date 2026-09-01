// Copyright 2024 keyball-custom contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "kb_settings.h"
#include "eeprom.h"
#include <stdbool.h>
#include <string.h>

static kb_settings_t g_cache;
static bool          g_loaded = false;

// scroll_layer/gesture_layer/gesture_th_h/gesture_th_v のRAMキャッシュ・読み込み
// 済みフラグは各機能のstatic変数（下部）で管理している。write_defaults()から
// 直接書き換えるため、前方宣言しておく。
static uint8_t g_scroll_layer;
static bool    g_scroll_loaded;
#ifdef GESTURE_ENABLE
static uint8_t g_gesture_layer;
static bool    g_gesture_loaded;
static uint8_t g_gesture_th_h;
static bool    g_gesture_th_h_loaded;
static uint8_t g_gesture_th_v;
static bool    g_gesture_th_v_loaded;
#endif

// マジックナンバー不一致時（未初期化・別フォーマット・データ破損）に、
// このファイルが管理する設定を全て安全な既定値へ書き戻し、RAMキャッシュにも反映する。
static void kb_settings_write_defaults(void) {
    memset(&g_cache, 0, sizeof(g_cache));
    g_cache.tapping_term  = KB_SETTINGS_DEFAULT_TT;
    g_cache.aml_layer     = 1;
    g_cache.aml_timeout   = 650;
    g_cache.aml_threshold = 10;
    g_cache.gesture[0]    = KB_GESTURE_DEFAULT_UP;
    g_cache.gesture[1]    = KB_GESTURE_DEFAULT_DOWN;
    g_cache.gesture[2]    = KB_GESTURE_DEFAULT_LEFT;
    g_cache.gesture[3]    = KB_GESTURE_DEFAULT_RIGHT;
    eeprom_write_block(&g_cache, (void *)(uintptr_t)KB_SETTINGS_EEPROM_BASE, sizeof(kb_settings_t));
    g_loaded = true;

    g_scroll_layer = 3;  // 正規ファームウェアと同じ既定値（レイヤー3でスクロール）
    eeprom_write_byte((uint8_t *)(uintptr_t)KB_SCROLL_LAYER_EEPROM, g_scroll_layer);
    g_scroll_loaded = true;

#ifdef GESTURE_ENABLE
    g_gesture_layer = KB_LAYER_NONE;
    eeprom_write_byte((uint8_t *)(uintptr_t)KB_GESTURE_LAYER_EEPROM, g_gesture_layer);
    g_gesture_loaded = true;

    g_gesture_th_h = KB_GESTURE_TH_DEFAULT;
    eeprom_write_byte((uint8_t *)(uintptr_t)KB_GESTURE_TH_H_EEPROM, g_gesture_th_h);
    g_gesture_th_h_loaded = true;

    g_gesture_th_v = KB_GESTURE_TH_DEFAULT;
    eeprom_write_byte((uint8_t *)(uintptr_t)KB_GESTURE_TH_V_EEPROM, g_gesture_th_v);
    g_gesture_th_v_loaded = true;
#endif

    eeprom_write_byte((uint8_t *)(uintptr_t)KB_SETTINGS_MAGIC_EEPROM,     KB_SETTINGS_MAGIC_VALUE_1);
    eeprom_write_byte((uint8_t *)(uintptr_t)(KB_SETTINGS_MAGIC_EEPROM+1), KB_SETTINGS_MAGIC_VALUE_2);
}

// 初回アクセス時に一度だけ、マジックナンバー(2バイト)を確認する。不一致なら
// 全設定を既定値へリセットする（＝別ファームや旧フォーマットの残存データを
// 正規の設定として誤読することを防ぐ）。
static void kb_settings_ensure_magic(void) {
    static bool checked = false;
    if (checked) return;
    checked = true;
    if (eeprom_read_byte((const uint8_t *)(uintptr_t)KB_SETTINGS_MAGIC_EEPROM)     != KB_SETTINGS_MAGIC_VALUE_1 ||
        eeprom_read_byte((const uint8_t *)(uintptr_t)(KB_SETTINGS_MAGIC_EEPROM+1)) != KB_SETTINGS_MAGIC_VALUE_2) {
        kb_settings_write_defaults();
    }
}

void kb_settings_reset_all(void) {
    kb_settings_write_defaults();
}

kb_settings_t kb_settings_get(void) {
    kb_settings_ensure_magic();
    if (!g_loaded) {
        eeprom_read_block(&g_cache,
            (void *)(uintptr_t)KB_SETTINGS_EEPROM_BASE,
            sizeof(kb_settings_t));
        // マジックナンバー検証済みなので基本的に正常値のはずだが、tapping_term
        // だけは影響が大きい(壊れると操作不能に近くなる)ため念のため残す。
        if (g_cache.tapping_term == 0xFFFF || g_cache.tapping_term < 50 || g_cache.tapping_term > 1000) {
            memset(&g_cache, 0, sizeof(g_cache));
            g_cache.tapping_term = KB_SETTINGS_DEFAULT_TT;
        }
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
uint8_t kb_scroll_layer_get(void) {
    kb_settings_ensure_magic();
    if (!g_scroll_loaded) {
        uint8_t v = eeprom_read_byte((const uint8_t *)(uintptr_t)KB_SCROLL_LAYER_EEPROM);
        if (v <= 7)                  g_scroll_layer = v;             // 0-7 = そのレイヤー
        else if (v == KB_LAYER_NONE) g_scroll_layer = KB_LAYER_NONE; // 明示的に「なし」
        else                         g_scroll_layer = 3;            // 0xFF未初期化 → 既定レイヤー3（正規ファームと同じ）
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
uint8_t kb_gesture_layer_get(void) {
    kb_settings_ensure_magic();
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
uint8_t kb_gesture_th_h_get(void) {
    kb_settings_ensure_magic();
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

uint8_t kb_gesture_th_v_get(void) {
    kb_settings_ensure_magic();
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
