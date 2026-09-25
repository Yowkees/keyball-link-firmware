// Copyright 2024 keyball-custom contributors
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <stdint.h>

// スロット数と共有バッファサイズ
#define MACRO_SLOT_COUNT    10
// 2026-09-25: Keyball61はdynamic_keymapが大きい分、マクロ領域の置き場所を
// 後ろにずらす必要があり（kb_settings.h・td_config.c参照）、AVRのEEPROM総容量
// 1024バイトに収めるためマクロバッファ自体も縮小する（400→240バイト）。
// Web UI側（keyball-configurator/src/lib/protocol.ts）もこの値を機種別に
// 把握しており、機種を変更したらそちらも合わせて更新すること。
#ifdef KB_EEPROM_LAYOUT_KEYBALL61
#define MACRO_BUFFER_SIZE   240   // 全スロット共有バッファ（バイト）Keyball61専用
#else
#define MACRO_BUFFER_SIZE   400   // 全スロット共有バッファ（バイト）
#endif

// バッファ内のアクションコード（VIA互換）
#define MACRO_ACTION_TAP    0x01  // キーをタップ: 0x01 hi lo
#define MACRO_ACTION_DOWN   0x02  // キーを押し続ける（マクロキー解放時に自動でUP）: 0x02 hi lo
#define MACRO_ACTION_DELAY  0x04  // 遅延: 0x04 hi lo (ms)
#define MACRO_ACTION_END    0x00  // マクロ終端

// HIDバッファ転送の1パケットあたりのデータ量
#define MACRO_CHUNK_SIZE    28

// バッファの一部をEEPROMから読み込む
void kb_macro_buffer_read(uint16_t offset, uint8_t *buf, uint8_t len);

// バッファの一部をEEPROMに書き込む
void kb_macro_buffer_write(uint16_t offset, const uint8_t *buf, uint8_t len);

// 指定スロットのマクロを再生する（タップ実行＋ホールド開始）
void kb_macro_play(uint8_t idx);

// ホールド中（DOWN指定）のキーをすべて解放する（マクロキーを離したとき呼ぶ）
void kb_macro_release(void);
