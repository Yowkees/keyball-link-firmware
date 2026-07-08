// Copyright 2024 keyball-custom contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include QMK_KEYBOARD_H
#include "kb_hid.h"
#include "keyball.h"
#include "raw_hid.h"
#include "dynamic_keymap.h"
#include "eeconfig.h"
#ifdef TAP_DANCE_ENABLE
#    include "td_config.h"
#endif
#include "kb_settings.h"
#include "kb_macro.h"
#ifdef POINTING_DEVICE_AUTO_MOUSE_ENABLE
#    include "pointing_device_auto_mouse.h"
#endif
#ifdef KEYBALL_AML_THRESHOLD_RUNTIME
extern uint8_t kb_aml_threshold;
#endif

#define RAW_PACKET_SIZE 32

// GET_INFO レスポンス用のモデル番号変換
// KEYBALL_MODEL は 39/44/61 の数値で定義されている
static uint8_t get_model_id(void) {
    return (uint8_t)KEYBALL_MODEL;
}

void kb_hid_receive(uint8_t *data, uint8_t length) {
    uint8_t response[RAW_PACKET_SIZE] = {0};
    uint8_t cmd = data[0];
    response[0] = cmd;

    switch (cmd) {

        // 0x01: キーボード情報を返す
        // 応答: [cmd, model, layers, rows, cols, protocol_ver, status]
        case KB_HID_CMD_GET_INFO: {
            response[1] = get_model_id();
            response[2] = dynamic_keymap_get_layer_count();
            response[3] = MATRIX_ROWS;
            response[4] = MATRIX_COLS;
            response[5] = KB_HID_PROTOCOL_VERSION;
            response[6] = KB_HID_STATUS_OK;
            break;
        }

        // 0x02: 指定位置のキーコードを返す
        // 要求: [cmd, layer, row, col]
        // 応答: [cmd, layer, row, col, keycode_hi, keycode_lo, status]
        case KB_HID_CMD_GET_KEYCODE: {
            uint8_t  layer   = data[1];
            uint8_t  row     = data[2];
            uint8_t  col     = data[3];
            uint16_t keycode = dynamic_keymap_get_keycode(layer, row, col);
            response[1] = layer;
            response[2] = row;
            response[3] = col;
            response[4] = (keycode >> 8) & 0xFF;
            response[5] = keycode & 0xFF;
            response[6] = KB_HID_STATUS_OK;
            break;
        }

        // 0x03: 指定位置にキーコードを書き込む
        // 要求: [cmd, layer, row, col, keycode_hi, keycode_lo]
        // 応答: [cmd, status]
        case KB_HID_CMD_SET_KEYCODE: {
            uint8_t  layer   = data[1];
            uint8_t  row     = data[2];
            uint8_t  col     = data[3];
            uint16_t keycode = ((uint16_t)data[4] << 8) | data[5];
            dynamic_keymap_set_keycode(layer, row, col, keycode);
            response[1] = KB_HID_STATUS_OK;
            break;
        }

        // 0x04: トラックボール設定を返す
        // 応答: [cmd, cpi_index, scroll_div, scrollsnap_mode, status]
        case KB_HID_CMD_GET_TRACKBALL: {
            keyball_config_t config = {.raw = eeconfig_read_kb()};
            response[1] = config.cpi;
            response[2] = config.sdiv;
            response[3] = (uint8_t)keyball_get_scrollsnap_mode();
            response[4] = KB_HID_STATUS_OK;
            break;
        }

        // 0x05: トラックボール設定を変更する
        // 要求: [cmd, cpi_index, scroll_div, scrollsnap_mode]
        // 応答: [cmd, status]
        case KB_HID_CMD_SET_TRACKBALL: {
            uint8_t cpi_idx = data[1];
            uint8_t sdiv    = data[2];
            uint8_t ssnap   = data[3];
            keyball_config_t config = {.raw = eeconfig_read_kb()};
            config.cpi  = cpi_idx;
            config.sdiv = sdiv;
#if KEYBALL_SCROLLSNAP_ENABLE == 2
            if (ssnap <= 2) config.ssnap = ssnap;
#endif
            eeconfig_update_kb(config.raw);
            keyball_set_cpi(cpi_idx);
            keyball_set_scroll_div(sdiv);
            if (ssnap <= 2) keyball_set_scrollsnap_mode((keyball_scrollsnap_mode_t)ssnap);
            response[1] = KB_HID_STATUS_OK;
            break;
        }

        // 0x06: EEPROMに保存（dynamic_keymapは自動保存なのでflushed通知のみ）
        // 応答: [cmd, status]
        case KB_HID_CMD_SAVE: {
            eeconfig_update_kb(eeconfig_read_kb());
            response[1] = KB_HID_STATUS_OK;
            break;
        }

        // 0x08: キーマップをデフォルト（コンパイル済み）に戻す
        // 応答: [cmd, status]
        case KB_HID_CMD_RESET_KEYMAP: {
            dynamic_keymap_reset();
            response[1] = KB_HID_STATUS_OK;
            break;
        }

        // 0x09: アクセラレーション設定を返す
        // 応答: [cmd, accel, status]
        case KB_HID_CMD_GET_ACCEL: {
            response[1] = keyball_get_accel();
            response[2] = KB_HID_STATUS_OK;
            break;
        }

        // 0x0A: アクセラレーション設定を変更してEEPROMに保存する
        // 要求: [cmd, accel]
        // 応答: [cmd, status]
        case KB_HID_CMD_SET_ACCEL: {
            uint8_t accel = data[1];
            keyball_set_accel(accel);
            keyball_config_t config = {.raw = eeconfig_read_kb()};
            config.accel = keyball_get_accel();
            eeconfig_update_kb(config.raw);
            response[1] = KB_HID_STATUS_OK;
            break;
        }

#ifdef RGB_MATRIX_ENABLE
        // エフェクトID対応表（Web側の0-5をQMKモード番号に変換）
        // 0:オフ 1:単色 2:呼吸 3:レインボー 4:リップル 5:リップル（複数同時）
#define LED_EFFECT_COUNT 6
#ifdef RGB_MATRIX_CUSTOM_USER
        static const uint8_t LED_EFFECT_MAP[LED_EFFECT_COUNT] = {
            RGB_MATRIX_NONE,
            RGB_MATRIX_SOLID_COLOR,
            RGB_MATRIX_BREATHING,
            RGB_MATRIX_CYCLE_ALL,
            RGB_MATRIX_CUSTOM_SOLID_RIPPLE,
            RGB_MATRIX_CUSTOM_SOLID_RIPPLE,
        };
#else
        static const uint8_t LED_EFFECT_MAP[LED_EFFECT_COUNT] = {
            RGB_MATRIX_NONE,
            RGB_MATRIX_SOLID_COLOR,
            RGB_MATRIX_BREATHING,
            RGB_MATRIX_CYCLE_ALL,
            RGB_MATRIX_BREATHING,
            RGB_MATRIX_BREATHING,
        };
#endif

        // 0x0B: LED設定を返す
        // 応答: [cmd, effect_id, hue, sat, val, speed, status]
        case KB_HID_CMD_GET_LED: {
            uint8_t mode = rgb_matrix_get_mode();
            uint8_t effect_id = 0;
            for (uint8_t i = 0; i < LED_EFFECT_COUNT; i++) {
                if (LED_EFFECT_MAP[i] == mode) { effect_id = i; break; }
            }
            response[1] = effect_id;
            response[2] = rgb_matrix_get_hue();
            response[3] = rgb_matrix_get_sat();
            response[4] = rgb_matrix_get_val();
            response[5] = rgb_matrix_get_speed();
            response[6] = KB_HID_STATUS_OK;
            break;
        }

        // 0x0C: LED設定を変更してEEPROMに保存する
        // 要求: [cmd, effect_id, hue, sat, val, speed]
        // 応答: [cmd, status]
        case KB_HID_CMD_SET_LED: {
            uint8_t effect_id = data[1] < LED_EFFECT_COUNT ? data[1] : 0;
            rgb_matrix_mode(LED_EFFECT_MAP[effect_id]);
            rgb_matrix_sethsv(data[2], data[3], data[4]);
            rgb_matrix_set_speed(data[5]);
            response[1] = KB_HID_STATUS_OK;
            break;
        }
#elif defined(RGBLIGHT_ENABLE)
        // エフェクトID対応表（0:オフ 1:単色 2:呼吸 3:レインボー）
#define LED_EFFECT_COUNT 4
        static const uint8_t LED_EFFECT_MAP[LED_EFFECT_COUNT] = {
            0,
            RGBLIGHT_MODE_STATIC_LIGHT,
            RGBLIGHT_MODE_BREATHING,
            RGBLIGHT_MODE_RAINBOW_MOOD,
        };

        // 0x0B: LED設定を返す
        // 応答: [cmd, effect_id, hue, sat, val, speed, status]
        case KB_HID_CMD_GET_LED: {
            uint8_t mode = rgblight_is_enabled() ? rgblight_get_mode() : 0;
            uint8_t effect_id = 0;
            for (uint8_t i = 1; i < LED_EFFECT_COUNT; i++) {
                if (LED_EFFECT_MAP[i] == mode) { effect_id = i; break; }
            }
            response[1] = effect_id;
            response[2] = rgblight_get_hue();
            response[3] = rgblight_get_sat();
            response[4] = rgblight_get_val();
            response[5] = rgblight_get_speed();
            response[6] = KB_HID_STATUS_OK;
            break;
        }

        // 0x0C: LED設定を変更してEEPROMに保存する
        // 要求: [cmd, effect_id, hue, sat, val, speed]
        // 応答: [cmd, status]
        case KB_HID_CMD_SET_LED: {
            uint8_t effect_id = data[1] < LED_EFFECT_COUNT ? data[1] : 0;
            if (effect_id == 0) {
                rgblight_disable();
            } else {
                rgblight_enable();
                rgblight_mode(LED_EFFECT_MAP[effect_id]);
                rgblight_sethsv(data[2], data[3], data[4]);
                rgblight_set_speed(data[5]);
            }
            response[1] = KB_HID_STATUS_OK;
            break;
        }
#endif

#ifdef TAP_DANCE_ENABLE
        // 0x0D: タップダンス設定を返す
        case KB_HID_CMD_GET_TD: {
            uint8_t idx = data[1];
            if (idx >= TD_SLOT_COUNT) { response[1] = KB_HID_STATUS_ERROR; break; }
            td_slot_t slot = td_config_get(idx);
            response[1] = idx;
            response[2] = (slot.tap  >> 8) & 0xFF; response[3] = slot.tap  & 0xFF;
            response[4] = (slot.hold >> 8) & 0xFF; response[5] = slot.hold & 0xFF;
            response[6] = (slot.dtap >> 8) & 0xFF; response[7] = slot.dtap & 0xFF;
            response[8] = slot.flags;
            response[9] = KB_HID_STATUS_OK;
            break;
        }

        // 0x0E: タップダンス設定を変更してEEPROMに保存する
        case KB_HID_CMD_SET_TD: {
            uint8_t idx = data[1];
            if (idx >= TD_SLOT_COUNT) { response[1] = KB_HID_STATUS_ERROR; break; }
            td_slot_t slot;
            slot.tap   = ((uint16_t)data[2] << 8) | data[3];
            slot.hold  = ((uint16_t)data[4] << 8) | data[5];
            slot.dtap  = ((uint16_t)data[6] << 8) | data[7];
            slot.flags = data[8];
            slot._pad  = 0;
            td_config_set(idx, &slot);
            response[1] = KB_HID_STATUS_OK;
            break;
        }
#endif // TAP_DANCE_ENABLE

        // 0x0F: 詳細設定を返す
        // 応答: [cmd, tt_hi, tt_lo, flags, aml_layer, aml_to_hi, aml_to_lo, status]
        case KB_HID_CMD_GET_SETTINGS: {
            kb_settings_t s = kb_settings_get();
            response[1] = (s.tapping_term >> 8) & 0xFF;
            response[2] = s.tapping_term & 0xFF;
            response[3] = s.flags;
            response[4] = s.aml_layer;
            response[5] = (s.aml_timeout >> 8) & 0xFF;
            response[6] = s.aml_timeout & 0xFF;
            response[7] = s.aml_threshold;
            response[8] = KB_HID_STATUS_OK;
            response[9] = kb_scroll_layer_get();  // スクロールレイヤー（0-7 / 0xFE=なし）
            break;
        }

        // 0x10: 詳細設定を変更してEEPROMに保存する
        // 要求: [cmd, tt_hi, tt_lo, flags, aml_layer, aml_to_hi, aml_to_lo, aml_threshold]
        // 応答: [cmd, status]
        case KB_HID_CMD_SET_SETTINGS: {
            kb_settings_t s = kb_settings_get();  // gesture等の既存値を保持してから上書き
            s.tapping_term  = ((uint16_t)data[1] << 8) | data[2];
            s.flags         = data[3];
            s.aml_layer     = data[4];
            s.aml_timeout   = ((uint16_t)data[5] << 8) | data[6];
            s.aml_threshold = data[7] ? data[7] : 10;
            kb_settings_set(&s);
            kb_scroll_layer_set(data[8]);  // スクロールレイヤー（0-7 / 0xFE=なし）
#ifdef AUTO_SHIFT_ENABLE
            if (s.flags & KB_FLAG_AUTO_SHIFT) autoshift_enable();
            else autoshift_disable();
#endif
#ifdef COMBO_ENABLE
            if (s.flags & KB_FLAG_COMBO) combo_enable();
            else combo_disable();
#endif
#ifdef POINTING_DEVICE_AUTO_MOUSE_ENABLE
            set_auto_mouse_layer(s.aml_layer);
            set_auto_mouse_timeout(s.aml_timeout);
            set_auto_mouse_enable(!(s.flags & KB_FLAG_AML_DISABLE));
#endif
#ifdef KEYBALL_AML_THRESHOLD_RUNTIME
            kb_aml_threshold = s.aml_threshold;
#endif
            response[1] = KB_HID_STATUS_OK;
            break;
        }

#ifdef GESTURE_ENABLE
        // 0x15: ジェスチャー設定を返す
        // 応答: [cmd, up_hi,up_lo, down_hi,down_lo, left_hi,left_lo, right_hi,right_lo, status, tap]
        case KB_HID_CMD_GET_GESTURE: {
            kb_settings_t s = kb_settings_get();
            for (uint8_t i = 0; i < 4; i++) {
                response[1 + i * 2] = (s.gesture[i] >> 8) & 0xFF;
                response[2 + i * 2] = s.gesture[i] & 0xFF;
            }
            response[9]  = KB_HID_STATUS_OK;
            response[10] = s.gesture_tap;          // タップ時の基本キーコード（0=なし）
            response[11] = kb_gesture_layer_get(); // ジェスチャーレイヤー（0-7 / 0xFE=なし）
            break;
        }

        // 0x16: ジェスチャー設定を変更してEEPROMに保存
        // 要求: [cmd, up_hi,up_lo, down_hi,down_lo, left_hi,left_lo, right_hi,right_lo, tap]
        case KB_HID_CMD_SET_GESTURE: {
            kb_settings_t s = kb_settings_get();
            for (uint8_t i = 0; i < 4; i++) {
                s.gesture[i] = ((uint16_t)data[1 + i * 2] << 8) | data[2 + i * 2];
            }
            s.gesture_tap = data[9];  // タップ時の基本キーコード（0=なし）
            kb_settings_set(&s);
            kb_gesture_layer_set(data[10]);  // ジェスチャーレイヤー（0-7 / 0xFE=なし）
            response[1] = KB_HID_STATUS_OK;
            break;
        }
#endif // GESTURE_ENABLE

        // 0x07: ブートローダーへジャンプ（ファームウェア書き込み用）
        // 応答を送信してから 500ms 後にリセット
        case KB_HID_CMD_REBOOT: {
            response[1] = KB_HID_STATUS_OK;
            raw_hid_send(response, RAW_PACKET_SIZE);
            wait_ms(500);
            bootloader_jump();
            return;  // raw_hid_send を二重に呼ばないよう return
        }

#ifdef RGB_MATRIX_ENABLE
        // 0x11: LED診断モード — 指定インデックスのLEDのみ点灯 (0xFF で終了)
        // LED_TEST エフェクトに切替え、hue フィールドにインデックスを格納して左右同期する
        // 要求: [cmd, led_index]  (0xFF = 終了)
        // 応答: [cmd, status]
        case KB_HID_CMD_TEST_LED: {
            uint8_t idx = data[1];
            if (idx == 0xFF) {
                rgb_matrix_mode_noeeprom(RGB_MATRIX_BREATHING);
            } else if (idx < RGB_MATRIX_LED_COUNT) {
#ifdef RGB_MATRIX_CUSTOM_USER
                rgb_matrix_mode_noeeprom(RGB_MATRIX_CUSTOM_LED_TEST);
                rgb_matrix_sethsv_noeeprom(idx, 255, 200);
#endif
            }
            response[1] = KB_HID_STATUS_OK;
            break;
        }
#endif

        // 0x12: 現在のキーマトリクス状態を返す
        // 応答: [cmd, rows, row0_mask, row1_mask, ..., row(N-1)_mask, status]
        // 各 row_mask の bit i が 1 のとき col i が押されている
        case KB_HID_CMD_GET_MATRIX: {
            uint8_t rows = MATRIX_ROWS;
            response[1] = rows;
            for (uint8_t r = 0; r < rows && r < 28; r++) {
                matrix_row_t row = matrix_get_row(r);
                response[2 + r] = (uint8_t)(row & 0xFF);
            }
            response[2 + rows] = KB_HID_STATUS_OK;
            break;
        }

        // 0x13: マクロバッファの一部を返す（VIA方式）
        // 要求: [cmd, offset_hi, offset_lo]
        // 応答: [cmd, offset_hi, offset_lo, data×28, status]
        case KB_HID_CMD_GET_MACRO: {
            uint16_t offset = ((uint16_t)data[1] << 8) | data[2];
            response[1] = data[1];
            response[2] = data[2];
            kb_macro_buffer_read(offset, &response[3], MACRO_CHUNK_SIZE);
            response[3 + MACRO_CHUNK_SIZE] = KB_HID_STATUS_OK;
            break;
        }

        // 0x14: マクロバッファの一部を書き込む（VIA方式）
        // 要求: [cmd, offset_hi, offset_lo, len, data×len]
        // 応答: [cmd, status]
        case KB_HID_CMD_SET_MACRO: {
            uint16_t offset = ((uint16_t)data[1] << 8) | data[2];
            uint8_t  len    = data[3] <= MACRO_CHUNK_SIZE ? data[3] : MACRO_CHUNK_SIZE;
            kb_macro_buffer_write(offset, &data[4], len);
            response[1] = KB_HID_STATUS_OK;
            break;
        }

        default:
            response[0] = 0xFF;
            response[1] = KB_HID_STATUS_ERROR;
            break;
    }

    raw_hid_send(response, RAW_PACKET_SIZE);
}
