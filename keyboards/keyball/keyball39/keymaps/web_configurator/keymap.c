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
static bool     g_gesture_cooldown = false;  // 発火直後は次の発火まで待つ（1スイング1回に制限）
static uint16_t g_gesture_cd_timer = 0;
#define GST_COOLDOWN_MS 350  // クールダウン時間(ms)
#endif

// clang-format off
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
  [0] = {
    {     19,     18,     12,     24,     28,      0 },
    {  17197,     15,     14,  16653,     11,      0 },
    {  16952,     55,     54,     16,     17,      0 },
    {    647,      0,      0,  16938,  16938,  16680 },
    {     20,     26,      8,     21,     23,      0 },
    {      4,     22,      7,      9,     10,      0 },
    {  16669,     27,      6,     25,      5,      0 },
    {    225,    227,    226,  17297,     44,  17040 },
  },
  [1] = {
    {     39,     38,     37,     36,     35,      0 },
    {     67,     66,     65,     64,     63,      0 },
    {    558,     47,     49,     48,     51,      0 },
    {    137,      0,      0,    209,      1,      1 },
    {     30,     31,     32,     33,     34,      0 },
    {     58,     59,     60,     61,     62,      0 },
    {     46,    549,     48,     49,     52,      0 },
    {      1,      1,      1,     41,     43,     76 },
  },
  [2] = {
    {      0,      0,    211,      0,      0,      0 },
    {  21027,    210,     82,    209,      0,      0 },
    {      0,     79,     81,     80,      0,      0 },
    {      0,      0,      0,     42,      1,      1 },
    {     84,     95,     96,     97,     86,      0 },
    {     85,     92,     93,     94,     87,      0 },
    {     98,     89,     90,     91,     99,      0 },
    {      1,      1,      1,      1,      1,      1 },
  },
  [3] = {
    {     86,     97,     96,     95,     84,      0 },
    {     87,     94,     93,     92,     85,      0 },
    {     99,     91,     90,     89,     98,      0 },
    {      0,      0,      0,    210,      1,      1 },
    {      0,      0,      0,      0,      0,      0 },
    {    260,    278,    263,    265,    266,      0 },
    {    285,    283,    262,    281,    261,      0 },
    {      1,      1,      1,      1,      1,      1 },
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
            g_gesture_cooldown = false;
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
            g_gesture_cooldown = false;
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

// ポインターアクセラレーション
void keyball_on_apply_motion_to_mouse_move(keyball_motion_t *m, report_mouse_t *r, bool is_left) {
    static int32_t acc[2][2];
    static int16_t ema[2][2];

    int16_t dx = m->y;
    int16_t dy = m->x;
    if (!is_left) { dx = -dx; dy = -dy; }

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
            acc[side][0] = 0; acc[side][1] = 0;
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
        acc[side][0] = 0; acc[side][1] = 0;
    }
    r->x = (int8_t)dx;
    r->y = (int8_t)dy;
    m->x = 0; m->y = 0;
}

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

layer_state_t layer_state_set_user(layer_state_t state) {
    uint8_t hl = get_highest_layer(state);
    keyball_set_scroll_mode(kb_scroll_layer_get() == hl);  // 設定レイヤーでスクロール（なし=0xFEは一致しない）
#ifdef GESTURE_ENABLE
    g_gesture_layer_on = (kb_gesture_layer_get() == hl);   // 設定レイヤーでジェスチャー
    if (!g_gesture_layer_on && !g_gesture_active && !g_gesture_pending) {
        g_gesture_acc_x = 0;
        g_gesture_acc_y = 0;
        g_gesture_cooldown = false;
    }
#endif
    return state;
}

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
        if (g_gesture_cooldown) {
            // 発火直後のクールダウン中は溜め込まない（1スイングで連続発火しない）
            if (timer_elapsed(g_gesture_cd_timer) > GST_COOLDOWN_MS) {
                g_gesture_cooldown = false;
            } else {
                g_gesture_acc_x = 0;
                g_gesture_acc_y = 0;
                mouse_report.x = 0;
                mouse_report.y = 0;
                return mouse_report;
            }
        }
        g_gesture_acc_x += mouse_report.x;
        g_gesture_acc_y += mouse_report.y;
        uint8_t th_h = kb_gesture_th_h_get();  // 横方向（左右）のしきい値
        uint8_t th_v = kb_gesture_th_v_get();  // 縦方向（上下）のしきい値
        if (g_gesture_acc_x > th_h || g_gesture_acc_x < -th_h ||
            g_gesture_acc_y > th_v || g_gesture_acc_y < -th_v) {
            int16_t ax = g_gesture_acc_x < 0 ? -g_gesture_acc_x : g_gesture_acc_x;
            int16_t ay = g_gesture_acc_y < 0 ? -g_gesture_acc_y : g_gesture_acc_y;
            kb_settings_t gs = kb_settings_get();
            uint16_t kc = (ax > ay)
                ? (g_gesture_acc_x > 0 ? gs.gesture[3] : gs.gesture[2])   // 右 : 左
                : (g_gesture_acc_y > 0 ? gs.gesture[1] : gs.gesture[0]);  // 下 : 上
            if (kc) tap_code16(kc);
            g_gesture_acc_x = 0;
            g_gesture_acc_y = 0;
            g_gesture_cooldown = true;
            g_gesture_cd_timer = timer_read();
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

void raw_hid_receive(uint8_t *data, uint8_t length) {
    kb_hid_receive(data, length);
}

#ifdef TAP_DANCE_ENABLE

static uint8_t td_outcome[TD_SLOT_COUNT];
#define TD_OUT_TAP  1
#define TD_OUT_HOLD 2
#define TD_OUT_DTAP 3

static bool td_is_mo(uint16_t kc, uint8_t *layer_out) {
    if (kc >= 0x5100 && kc <= 0x511F) { *layer_out = kc & 0x1F; return true; }
    return false;
}

static void td_handle_finished(tap_dance_state_t *state, uint8_t idx) {
    td_slot_t s = td_config_get(idx);
    uint8_t layer;
    if (state->count == 1 && !state->pressed) {
        td_outcome[idx] = TD_OUT_TAP;
        register_code16(s.tap); send_keyboard_report();
    } else if (state->count == 1 && state->pressed) {
        td_outcome[idx] = TD_OUT_HOLD;
        if (td_is_mo(s.hold, &layer)) layer_on(layer);
        else { register_code16(s.hold); send_keyboard_report(); }
    } else {
        td_outcome[idx] = TD_OUT_DTAP;
        register_code16(s.dtap ? s.dtap : s.tap); send_keyboard_report();
    }
}

static void td_handle_reset(tap_dance_state_t *state, uint8_t idx) {
    td_slot_t s = td_config_get(idx);
    uint8_t layer;
    switch (td_outcome[idx]) {
        case TD_OUT_TAP:  unregister_code16(s.tap);  break;
        case TD_OUT_HOLD:
            if (td_is_mo(s.hold, &layer)) layer_off(layer);
            else unregister_code16(s.hold);
            break;
        case TD_OUT_DTAP: unregister_code16(s.dtap ? s.dtap : s.tap); break;
    }
    td_outcome[idx] = 0;
}

#define DEFINE_TD_FN(n) \
    static void td_fin_##n(tap_dance_state_t *s, void *u) { td_handle_finished(s, n); } \
    static void td_rst_##n(tap_dance_state_t *s, void *u) { td_handle_reset(s, n); }

DEFINE_TD_FN(0) DEFINE_TD_FN(1) DEFINE_TD_FN(2) DEFINE_TD_FN(3)
DEFINE_TD_FN(4) DEFINE_TD_FN(5) DEFINE_TD_FN(6) DEFINE_TD_FN(7)

tap_dance_action_t tap_dance_actions[] = {
    [0] = ACTION_TAP_DANCE_FN_ADVANCED(NULL, td_fin_0, td_rst_0),
    [1] = ACTION_TAP_DANCE_FN_ADVANCED(NULL, td_fin_1, td_rst_1),
    [2] = ACTION_TAP_DANCE_FN_ADVANCED(NULL, td_fin_2, td_rst_2),
    [3] = ACTION_TAP_DANCE_FN_ADVANCED(NULL, td_fin_3, td_rst_3),
    [4] = ACTION_TAP_DANCE_FN_ADVANCED(NULL, td_fin_4, td_rst_4),
    [5] = ACTION_TAP_DANCE_FN_ADVANCED(NULL, td_fin_5, td_rst_5),
    [6] = ACTION_TAP_DANCE_FN_ADVANCED(NULL, td_fin_6, td_rst_6),
    [7] = ACTION_TAP_DANCE_FN_ADVANCED(NULL, td_fin_7, td_rst_7),
};

#endif // TAP_DANCE_ENABLE

#ifdef OLED_ENABLE
#    include "lib/oledkit/oledkit.h"
void oledkit_render_info_user(void) {
    keyball_oled_render_keyinfo();
    keyball_oled_render_ballinfo();
    keyball_oled_render_layerinfo();
}
#endif
