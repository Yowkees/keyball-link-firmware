// Copyright 2024 keyball-custom contributors
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

// USB HID Country Code: 15 = Japan (JIS)
// macOSがKeyballをJIS配列キーボードとして自動認識するために必要
// これによりユーザーが手動でplistを編集する必要がなくなる
#define USB_HID_KEYBOARD_COUNTRY_CODE 15

// Dynamic keymapのレイヤー数（4レイヤー固定）
#define DYNAMIC_KEYMAP_LAYER_COUNT 4

// トラックボールのデフォルトCPI
#define KEYBALL_CPI_DEFAULT 500

// スクロール量の全体基準を下げる（2=従来の半分の速さ）
// 別マウスと併用しPC側のスクロール量を上げても速くなりすぎないように
#define KEYBALL_SCROLL_DIV_BASE 2

// 自動マウスレイヤー
#define POINTING_DEVICE_AUTO_MOUSE_ENABLE
#define AUTO_MOUSE_DEFAULT_LAYER 1
// 発動しきい値（移動量）をランタイム変更可能にする
#define KEYBALL_AML_THRESHOLD_RUNTIME

#define TAP_CODE_DELAY 5

// 詳細設定: per-key オーバーライド（LED版では容量確保のため簡略化）
#define TAPPING_TERM           200
#ifndef LED_VERSION_BUILD
#define TAPPING_TERM_PER_KEY
#define PERMISSIVE_HOLD_PER_KEY
#endif

// 使用するRGBLIGHTエフェクトのみ有効化（フラッシュ節約）
#define RGBLIGHT_EFFECT_BREATHING
#define RGBLIGHT_EFFECT_RAINBOW_MOOD

// RGBLIGHT デフォルト設定
#define RGBLIGHT_DEFAULT_MODE  RGBLIGHT_MODE_BREATHING
#define RGBLIGHT_DEFAULT_HUE   170
#define RGBLIGHT_DEFAULT_SAT   255
#define RGBLIGHT_DEFAULT_VAL   128
#define RGBLIGHT_DEFAULT_SPD   128

// 最大輝度（消費電力抑制）
#ifndef RGBLIGHT_LIMIT_VAL
#    define RGBLIGHT_LIMIT_VAL 150
#endif
