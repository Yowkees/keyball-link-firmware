/*
Copyright 2022 @Yowkees
Copyright 2022 MURAOKA Taro (aka KoRoN, @kaoriya)

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include QMK_KEYBOARD_H

#include "lib/keyball/keyball.h"

//////////////////////////////////////////////////////////////////////////////

// clang-format off
matrix_row_t matrix_mask[MATRIX_ROWS] = {
    0b00111111,
    0b00111111,
    0b00111111,
    0b00111110,
    0b00111111,
    0b00111111,
    0b00111111,
    0b00111110,
};
// clang-format on

void keyball_on_adjust_layout(keyball_adjust_t v) {
#ifdef RGBLIGHT_ENABLE
    // adjust RGBLIGHT's clipping and effect ranges
    uint8_t lednum_this = keyball.this_have_ball ? 29 : 30;
    uint8_t lednum_that = !keyball.that_enable ? 0 : keyball.that_have_ball ? 29 : 30;
    // 2026-10-07修正: 以前は右手側で clipping_start_pos を「左手のLED数」にしていたが、
    // 今のQMKのrgblightはエフェクト範囲の全LED（0〜左右合計-1）について
    // ws2812_set_color(index - clipping_start_pos, ...) を呼ぶため、右手側では
    // 0〜(左手のLED数-1) の分が負数→uint8_tの大きな値になり、ws2812のバッファの外
    // （OLEDの画面バッファなど）を毎フレーム書き換えていた。ブリージングで右手を
    // マスターにするとOLEDの表示が乱れる、左手のLEDの一部が消えるなどの原因（本人報告）。
    // AVR版で使えるエフェクト（ソリッド・ブリージング・レインボームード）は全LEDが同じ色の
    // ため、左右とも先頭から数えても見た目は変わらない。常に0から数えてはみ出しを防ぐ。
    (void)lednum_that;
    rgblight_set_clipping_range(0, lednum_this);
    rgblight_set_effect_range(0, lednum_this + lednum_that);
#endif
}
