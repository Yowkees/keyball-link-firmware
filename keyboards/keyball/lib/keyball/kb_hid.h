// Copyright 2024 keyball-custom contributors
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <stdint.h>
#include <stdbool.h>

// WebHID通信プロトコルのコマンドID
#define KB_HID_CMD_GET_INFO       0x01  // キーボード情報取得
#define KB_HID_CMD_GET_KEYCODE    0x02  // キーコード取得
#define KB_HID_CMD_SET_KEYCODE    0x03  // キーコード設定
#define KB_HID_CMD_GET_TRACKBALL  0x04  // トラックボール設定取得
#define KB_HID_CMD_SET_TRACKBALL  0x05  // トラックボール設定変更
#define KB_HID_CMD_SAVE           0x06  // EEPROMに保存
#define KB_HID_CMD_REBOOT         0x07  // ブートローダーへリセット
#define KB_HID_CMD_RESET_KEYMAP   0x08  // キーマップをデフォルトに戻す（EEPROM再初期化）
#define KB_HID_CMD_GET_ACCEL      0x09  // アクセラレーション設定取得
#define KB_HID_CMD_SET_ACCEL      0x0A  // アクセラレーション設定変更
#define KB_HID_CMD_GET_LED        0x0B  // LED設定取得
#define KB_HID_CMD_SET_LED        0x0C  // LED設定変更
#define KB_HID_CMD_GET_TD         0x0D  // タップダンス設定取得
#define KB_HID_CMD_SET_TD         0x0E  // タップダンス設定変更
#define KB_HID_CMD_GET_SETTINGS   0x0F  // キーボード詳細設定取得
#define KB_HID_CMD_SET_SETTINGS   0x10  // キーボード詳細設定変更
#define KB_HID_CMD_TEST_LED       0x11  // LED診断: 指定LEDのみ点灯 (0xFF=終了)
#define KB_HID_CMD_GET_MATRIX     0x12  // マトリクス状態取得: 各行のビットマスクを返す
#define KB_HID_CMD_GET_MACRO      0x13  // マクロ取得: [cmd,slot] → [cmd,slot,count,kc×6,status]
#define KB_HID_CMD_SET_MACRO      0x14  // マクロ設定: [cmd,slot,count,kc×6] → [cmd,status]
#define KB_HID_CMD_GET_GESTURE    0x15  // ジェスチャー設定取得（GESTURE_ENABLE時のみ応答）
#define KB_HID_CMD_SET_GESTURE    0x16  // ジェスチャー設定変更（GESTURE_ENABLE時のみ応答）

// ステータスコード
#define KB_HID_STATUS_OK    0x00
#define KB_HID_STATUS_ERROR 0x01

// プロトコルバージョン
#define KB_HID_PROTOCOL_VERSION 0x01

void kb_hid_receive(uint8_t *data, uint8_t length);
