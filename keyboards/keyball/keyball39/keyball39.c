/*
Copyright 2021 @Yowkees
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
    0b00011111,
    0b00011111,
    0b00011111,
    0b00111111,
    0b00011111,
    0b00011111,
    0b00011111,
    0b00111111,
};
// clang-format on

void keyball_on_adjust_layout(keyball_adjust_t v) {
#ifdef RGBLIGHT_ENABLE
    // adjust RGBLIGHT's clipping and effect ranges
    // 2026-10-07: 左右とも、LEDバッファ全体（左右合計の数）を書き換える範囲にする。
    // 以前は左右・ボールの有無ごとにLEDの数と開始位置を変えていたが、
    //  - 右手側の開始位置を「左手のLED数」にすると、今のQMKのrgblightでは書き込み先が
    //    負数→バッファの外になり、OLEDの画面バッファなどを壊していた
    //  - 起動直後（左右の情報交換の前）は数が確定しておらず、ソリッドなど一度しか書かない
    //    エフェクトで一部のLEDが消えたままになっていた（本人報告: 左手の27〜29番）
    // AVR版のエフェクト（ソリッド・ブリージング・レインボームード）は全LEDが同じ色なので、
    // 全範囲を書いても見た目は変わらない（各基板は自分につながっている数だけ光る）。
    rgblight_set_clipping_range(0, RGBLIGHT_LED_COUNT);
    rgblight_set_effect_range(0, RGBLIGHT_LED_COUNT);
#endif
}
