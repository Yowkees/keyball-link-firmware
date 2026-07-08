// Copyright 2024 keyball-custom contributors
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <stdint.h>

// タップダンスのスロット数（最大8スロット: TD(0)〜TD(7)）
#define TD_SLOT_COUNT 8

// 1スロットの設定（8バイト）
typedef struct {
    uint16_t tap;   // シングルタップ時のキーコード
    uint16_t hold;  // ホールド時のキーコード
    uint16_t dtap;  // ダブルタップ時のキーコード（0 = シングルタップと同じ）
    uint8_t  flags; // bit0: スロット有効
    uint8_t  _pad;
} __attribute__((packed)) td_slot_t;

// EEPROMからスロット設定を読み込む
td_slot_t td_config_get(uint8_t idx);

// EEPROMにスロット設定を書き込む
void td_config_set(uint8_t idx, const td_slot_t *slot);
