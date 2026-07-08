// Copyright 2024 keyball-custom contributors
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <stdint.h>

// TD config (0x0200-0x023F, 64 bytes) の直後に配置
// kb_settings は 16バイト（0x0240-0x024F）。マクロ領域(0x0250)の直前まで。
#define KB_SETTINGS_EEPROM_BASE  0x0240
#define KB_SETTINGS_DEFAULT_TT   200  // デフォルト Tapping Term (ms)

// ジェスチャーのデフォルト割り当て（macブラウザ標準・修飾子付きキーコード）
// 0x0800=LGUI(Cmd), 0x0200=LSFT, KC_LBRC=0x2F, KC_RBRC=0x30
#define KB_GESTURE_DEFAULT_UP    0x0A2F  // Cmd+Shift+[ 前のタブ
#define KB_GESTURE_DEFAULT_DOWN  0x0A30  // Cmd+Shift+] 次のタブ
#define KB_GESTURE_DEFAULT_LEFT  0x082F  // Cmd+[ 戻る
#define KB_GESTURE_DEFAULT_RIGHT 0x0830  // Cmd+] 進む

typedef struct {
    uint16_t tapping_term;   // 50-1000ms
    uint8_t  flags;          // KB_FLAG_* ビットフィールド
    uint8_t  aml_layer;      // 自動マウスレイヤーの対象レイヤー（0-7）
    uint16_t aml_timeout;    // 自動マウスレイヤーのタイムアウト(ms)
    uint8_t  aml_threshold;  // 自動マウスレイヤーの発動しきい値（移動量。小さいほど敏感）
    uint8_t  gesture_tap;    // ジェスチャーキーをタップした時に送る基本キーコード（0=なし=長押し専用）
    uint16_t gesture[4];     // ジェスチャー割り当て [0]上 [1]下 [2]左 [3]右（0=未設定→デフォルト）
} __attribute__((packed)) kb_settings_t;

#define KB_FLAG_AUTO_SHIFT       (1 << 0)
#define KB_FLAG_COMBO            (1 << 1)
#define KB_FLAG_PERMISSIVE_HOLD  (1 << 2)
#define KB_FLAG_RETRO_TAPPING    (1 << 3)
#define KB_FLAG_SCROLL_INV_V     (1 << 4)  // 縦スクロール反転
#define KB_FLAG_SCROLL_INV_H     (1 << 5)  // 横スクロール反転
#define KB_FLAG_AML_DISABLE      (1 << 6)  // 自動マウスレイヤー無効（0=有効・後方互換）

// EEPROMから読み込む（初回のみ; 以降はRAMキャッシュを返す）
kb_settings_t kb_settings_get(void);

// EEPROMに書き込みRAMキャッシュも更新する
void kb_settings_set(const kb_settings_t *s);

// ── トラックボール動作レイヤー（kb_settings構造体は満杯のため、EEPROM末尾の
//    空き領域 0x03E0- に格納。マクロ領域は 0x0250-0x03DF なので衝突しない）──
#define KB_SCROLL_LAYER_EEPROM   0x03E0  // スクロールレイヤー保存先
#define KB_GESTURE_LAYER_EEPROM  0x03E1  // ジェスチャーレイヤー保存先
#define KB_LAYER_NONE            0xFE    // 「なし」を表す値（0xFF=未初期化と区別）

// スクロールレイヤー（0-7=そのレイヤーでスクロール / KB_LAYER_NONE=無効。既定3）
uint8_t kb_scroll_layer_get(void);
void    kb_scroll_layer_set(uint8_t v);

#ifdef GESTURE_ENABLE
// ジェスチャーレイヤー（0-7=そのレイヤーでジェスチャー / KB_LAYER_NONE=なし。既定なし）
uint8_t kb_gesture_layer_get(void);
void    kb_gesture_layer_set(uint8_t v);
#endif
