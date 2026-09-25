// Copyright 2024 keyball-custom contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "td_config.h"
#include "eeprom.h"
#include <string.h>

// タップダンス設定をEEPROMの 0x0200 番地以降に保存
// dynamic_keymap は先頭〜約 0x01C0 を使うため、0x0200 からは安全…という前提だったが、
// Keyball61はマトリクスが大きく(10行×8列)dynamic_keymapが0x02A5番地まで達するため、
// 0x0200のままでは重なってしまう。KB_EEPROM_LAYOUT_KEYBALL61（Keyball61の
// keymaps/web_configurator/rules.mkでOPT_DEFSとして定義）が立っていれば、
// dynamic_keymapの末尾より後ろの0x02B0番地から確保する
// （2026-09-25発覚のキーマップ破損不具合の修正）。
#ifdef KB_EEPROM_LAYOUT_KEYBALL61
#define TD_EEPROM_BASE 0x02B0
#else
#define TD_EEPROM_BASE 0x0200
#endif

td_slot_t td_config_get(uint8_t idx) {
    td_slot_t slot;
    if (idx >= TD_SLOT_COUNT) {
        memset(&slot, 0, sizeof(slot));
        return slot;
    }
    eeprom_read_block(&slot,
        (void *)(uintptr_t)(TD_EEPROM_BASE + idx * sizeof(td_slot_t)),
        sizeof(td_slot_t));
    return slot;
}

void td_config_set(uint8_t idx, const td_slot_t *slot) {
    if (idx >= TD_SLOT_COUNT) return;
    eeprom_write_block(slot,
        (void *)(uintptr_t)(TD_EEPROM_BASE + idx * sizeof(td_slot_t)),
        sizeof(td_slot_t));
}
