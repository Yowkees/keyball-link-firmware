// Copyright 2024 keyball-custom contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include QMK_KEYBOARD_H
#include "quantum.h"
#include "lib/keyball/kb_hid.h"
#include "lib/keyball/td_config.h"
#include "lib/keyball/kb_settings.h"
#include "lib/keyball/kb_macro.h"

#ifdef KEYBALL_AML_THRESHOLD_RUNTIME
extern uint8_t kb_aml_threshold;
#endif

#ifdef GESTURE_ENABLE
// ジェスチャー: ホールド中のトラックボール移動を累積し方向で判定して送出。
// タップ（短押し）時は gesture_tap に設定した通常キーを送る兼用キー。
static bool     g_gesture_active  = false;  // ジェスチャー発動中（確定）
static bool     g_gesture_pending = false;  // 押下直後、タップ/ホールド未確定
static bool     g_gesture_layer_on = false; // ジェスチャーレイヤーに滞在中
static uint8_t  g_gesture_tap_kc  = 0;      // タップ時に送る基本キーコード（0=なし）
static uint16_t g_gesture_timer   = 0;      // 押下時刻（ホールド判定用）
static int16_t  g_gesture_acc_x   = 0;
static int16_t  g_gesture_acc_y   = 0;
#endif

// clang-format off
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
  [0] = {
    {     45,     39,     38,      0,     37,     36,     35,      0 },
    {     47,     19,     18,      0,     12,     24,     28,      0 },
    {     52,  17203,     15,      0,     14,     13,     11,      0 },
    {    647,     56,     55,      0,     54,     16,     17,     49 },
    {  17289,    229,    226,      0,    227,  16785,  16938,  16680 },
    {     41,     30,     31,      0,     32,     33,     34,      0 },
    {     43,     20,     26,      0,      8,     21,     23,      0 },
    {    224,      4,     22,      0,      7,      9,     10,      0 },
    {     57,     29,     27,      0,      6,     25,      5,     48 },
    {     74,    225,    226,      0,    227,  17297,     44,  17040 },
  },
  [1] = {
    {     69,     68,     67,      0,     66,     65,     64,      0 },
    {      0,      0,      0,      0,    211,      0,      0,      0 },
    {      0,  21027,    210,      0,     82,    209,      0,      0 },
    {      0,      0,     79,      0,     81,     80,      0,      0 },
    {      0,      0,     39,      0,     55,      1,      1,      1 },
    {     58,     59,     60,      0,     61,     62,     63,      0 },
    {      0,      0,      0,      0,      0,      0,      0,      0 },
    {      0,      0,      0,      0,      0,      0,      0,      0 },
    {      0,      0,      0,      0,      0,      0,      0,      0 },
    {      0,      0,      0,      0,      0,      1,      1,      1 },
  },
  [2] = {
    {      0,     84,     97,      0,     96,     95,      0,      0 },
    {      0,     85,     94,      0,     93,     92,      0,      0 },
    {      0,     86,     91,      0,     90,     89,      0,      0 },
    {      0,     87,     88,      0,     99,     98,      0,      0 },
    {      0,      0,      1,      0,      1,      1,      1,      1 },
    {      1,      0,      0,      0,      0,      0,      0,      0 },
    {      1,      0,      0,      0,      0,      0,      0,      0 },
    {      1,      0,      0,      0,      0,      0,      0,      0 },
    {      1,      0,      0,      0,      0,      0,      0,      0 },
    {      1,      1,      1,      0,      1,      1,      1,      1 },
  },
  [3] = {
    {      0,      0,      0,      0,      0,      0,      0,      0 },
    {      0,      0,      0,      0,      0,      0,      0,      0 },
    {      0,      0,      0,      0,      0,      0,      0,      0 },
    {      0,      0,      0,      0,      0,      0,      0,      0 },
    {      0,      0,   2608,   2607,   2096,   2049,      0,      0 },
    {      0,      0,      0,      0,      0,      0,      0,      0 },
    {      0,      0,      0,      0,      0,      0,      0,      0 },
    {      0,      0,      0,      0,      0,      0,      0,      0 },
    {      0,      0,      0,      0,      0,      0,      0,      0 },
    {      0,      0,      0,      0,      0,      0,      0,      0 },
  },
};
// clang-format on

// マクロキー再生（QK_MACRO_0〜QK_MACRO_15 = 0x7700〜0x770F）
bool process_record_user(uint16_t keycode, keyrecord_t *record) {
#ifdef GESTURE_ENABLE
    if (keycode == GST_HOLD) {
        if (record->event.pressed) {
            uint8_t tk = kb_settings_get().gesture_tap;
            g_gesture_acc_x = 0;
            g_gesture_acc_y = 0;
            if (tk == 0) {
                // タップキー未設定 → 従来通りホールド専用
                g_gesture_active  = true;
                g_gesture_pending = false;
            } else {
                // タップ/ホールド兼用 → 判定を保留
                g_gesture_tap_kc  = tk;
                g_gesture_timer   = timer_read();
                g_gesture_pending = true;
                g_gesture_active  = false;
            }
        } else {
            // 離した: 保留のまま（＝短押し）ならタップキーを送る
            if (g_gesture_pending) tap_code(g_gesture_tap_kc);
            g_gesture_active  = false;
            g_gesture_pending = false;
            g_gesture_acc_x   = 0;
            g_gesture_acc_y   = 0;
        }
        return false;
    }
#endif
    if (keycode >= QK_MACRO && keycode <= QK_MACRO_MAX) {
        if (record->event.pressed) {
            kb_macro_play(keycode - QK_MACRO);   // タップ実行＋ホールド開始
        } else {
            kb_macro_release();                  // ホールド中のキーを解放
        }
        return false;
    }
    return true;
}

layer_state_t layer_state_set_user(layer_state_t state) {
    uint8_t hl = get_highest_layer(state);
    keyball_set_scroll_mode(kb_scroll_layer_get() == hl);  // 設定レイヤーでスクロール（なし=0xFEは一致しない）
#ifdef GESTURE_ENABLE
    g_gesture_layer_on = (kb_gesture_layer_get() == hl);   // 設定レイヤーでジェスチャー
    if (!g_gesture_layer_on && !g_gesture_active && !g_gesture_pending) {
        g_gesture_acc_x = 0;
        g_gesture_acc_y = 0;
    }
#endif
    return state;
}

void raw_hid_receive(uint8_t *data, uint8_t length) {
    kb_hid_receive(data, length);
}

// EMAフィルタ + 滑らかな2乗加速カーブ（LED版では容量確保のため無効＝デフォルト挙動）
#ifndef LED_VERSION_BUILD
void keyball_on_apply_motion_to_mouse_move(keyball_motion_t *m, report_mouse_t *r, bool is_left) {
    static int32_t acc[2][2];
    static int16_t ema[2][2];

    int16_t dx = m->y;
    int16_t dy = m->x;
    if (!is_left) {
        dx = -dx;
        dy = -dy;
    }

    uint8_t side = is_left ? 0 : 1;

    ema[side][0] += dx - ema[side][0] / 4;
    ema[side][1] += dy - ema[side][1] / 4;
    dx = ema[side][0] / 4;
    dy = ema[side][1] / 4;

    uint8_t accel = keyball_get_accel();

    if (accel > 0) {
        int16_t abs_dx = dx < 0 ? -dx : dx;
        int16_t abs_dy = dy < 0 ? -dy : dy;
        int16_t speed  = abs_dx > abs_dy ? abs_dx : abs_dy;

        if (speed == 0) {
            acc[side][0] = 0;
            acc[side][1] = 0;
        } else {
            uint8_t scale = 64 / accel;

            acc[side][0] += (int32_t)dx * speed;
            acc[side][1] += (int32_t)dy * speed;

            int32_t out_x = acc[side][0] / scale;
            int32_t out_y = acc[side][1] / scale;
            acc[side][0] -= out_x * (int32_t)scale;
            acc[side][1] -= out_y * (int32_t)scale;

            dx = out_x < -127 ? -127 : out_x > 127 ? 127 : (int16_t)out_x;
            dy = out_y < -127 ? -127 : out_y > 127 ? 127 : (int16_t)out_y;
        }
    } else {
        acc[side][0] = 0;
        acc[side][1] = 0;
    }

    r->x = (int8_t)dx;
    r->y = (int8_t)dy;
    m->x = 0;
    m->y = 0;
}
#endif

void keyboard_post_init_user(void) {
    kb_settings_t s = kb_settings_get();
#ifdef AUTO_SHIFT_ENABLE
    if (s.flags & KB_FLAG_AUTO_SHIFT) autoshift_enable();
    else autoshift_disable();
#endif
#ifdef POINTING_DEVICE_AUTO_MOUSE_ENABLE
    set_auto_mouse_layer(s.aml_layer);
    set_auto_mouse_timeout(s.aml_timeout);
    set_auto_mouse_enable(!(s.flags & KB_FLAG_AML_DISABLE));
#endif
#ifdef KEYBALL_AML_THRESHOLD_RUNTIME
    kb_aml_threshold = s.aml_threshold;
#endif
    (void)s;
}

#ifndef LED_VERSION_BUILD
uint16_t get_tapping_term(uint16_t keycode, keyrecord_t *record) {
    uint16_t tt = kb_settings_get().tapping_term;
    return (tt >= 50 && tt <= 1000) ? tt : TAPPING_TERM;
}

bool get_permissive_hold(uint16_t keycode, keyrecord_t *record) {
    return (kb_settings_get().flags & KB_FLAG_PERMISSIVE_HOLD) != 0;
}
#endif

#ifdef GESTURE_ENABLE
// ホールド判定: 兼用キーを押しっぱなしが Tapping Term を超えたらジェスチャー確定
void matrix_scan_user(void) {
    if (g_gesture_pending) {
        uint16_t tt = kb_settings_get().tapping_term;
        if (tt < 50 || tt > 1000) tt = TAPPING_TERM;
        if (timer_elapsed(g_gesture_timer) > tt) {
            g_gesture_active  = true;
            g_gesture_pending = false;
        }
    }
}
#endif

// スクロール方向の反転（EEPROM設定に応じて符号を反転）
report_mouse_t pointing_device_task_user(report_mouse_t mouse_report) {
#ifdef GESTURE_ENABLE
    // 保留中（兼用キー押下直後）: 大きく振ったらジェスチャーと確定
    if (g_gesture_pending) {
        g_gesture_acc_x += mouse_report.x;
        g_gesture_acc_y += mouse_report.y;
        int16_t mx = g_gesture_acc_x < 0 ? -g_gesture_acc_x : g_gesture_acc_x;
        int16_t my = g_gesture_acc_y < 0 ? -g_gesture_acc_y : g_gesture_acc_y;
        if (mx > 12 || my > 12) {       // 明確に振った → ジェスチャー確定
            g_gesture_active  = true;
            g_gesture_pending = false;
        }
        mouse_report.x = 0;  // 判定中はカーソルを動かさない
        mouse_report.y = 0;
        return mouse_report;
    }
    if (g_gesture_active || g_gesture_layer_on) {
        g_gesture_acc_x += mouse_report.x;
        g_gesture_acc_y += mouse_report.y;
        const int16_t GST_TH = 50;  // 発動しきい値（移動量）
        if (g_gesture_acc_x > GST_TH || g_gesture_acc_x < -GST_TH ||
            g_gesture_acc_y > GST_TH || g_gesture_acc_y < -GST_TH) {
            int16_t ax = g_gesture_acc_x < 0 ? -g_gesture_acc_x : g_gesture_acc_x;
            int16_t ay = g_gesture_acc_y < 0 ? -g_gesture_acc_y : g_gesture_acc_y;
            kb_settings_t gs = kb_settings_get();
            uint16_t kc = (ax > ay)
                ? (g_gesture_acc_x > 0 ? gs.gesture[3] : gs.gesture[2])   // 右 : 左
                : (g_gesture_acc_y > 0 ? gs.gesture[1] : gs.gesture[0]);  // 下 : 上
            if (kc) tap_code16(kc);
            g_gesture_acc_x = 0;
            g_gesture_acc_y = 0;
        }
        mouse_report.x = 0;  // ジェスチャー中はカーソルを動かさない
        mouse_report.y = 0;
        return mouse_report;
    }
#endif
    if (mouse_report.h != 0 || mouse_report.v != 0) {
        uint8_t flags = kb_settings_get().flags;
        if (flags & KB_FLAG_SCROLL_INV_V) mouse_report.v = -mouse_report.v;
        if (flags & KB_FLAG_SCROLL_INV_H) mouse_report.h = -mouse_report.h;
    }
    return mouse_report;
}

#ifdef OLED_ENABLE
#    include "lib/oledkit/oledkit.h"

void oledkit_render_info_user(void) {
    keyball_oled_render_keyinfo();
    keyball_oled_render_ballinfo();
    keyball_oled_render_layerinfo();
}
#endif
