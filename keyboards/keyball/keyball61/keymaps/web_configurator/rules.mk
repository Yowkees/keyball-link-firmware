# Web Configurator keymap for Keyball61
# WebHID通信を有効化
RAW_ENABLE = yes

# Viaのraw_hid_receiveが独自プロトコルと競合するため無効化
VIA_ENABLE = no

# EEPROMへのキーマップ保存を有効化
DYNAMIC_KEYMAP_ENABLE = yes

# 2026-09-25: Keyball61はマトリクスが大きく(10行×8列)、dynamic_keymap用の
# EEPROM領域がKeyball39/44より大きい(640バイト、0x02A5番地まで)。
# lib/keyball/kb_settings.h・td_config.c・kb_macro.h/.cが使う独自EEPROM領域の
# デフォルト開始地点(0x0200)はこれより手前にあり、詳細設定やマクロを保存する
# たびにキーマップの一部が上書きされて壊れる不具合があった。このフラグを
# 立てることで、上記ファイルがKeyball61専用の（dynamic_keymapの後ろにずらした）
# アドレスを使うようになる。config.hではなくOPT_DEFSで渡すのは、
# kb_settings.c・td_config.cがconfig.hをincludeしていないため
# （GESTURE_ENABLE等、既存の同種フラグと同じ方式）。
OPT_DEFS += -DKB_EEPROM_LAYOUT_KEYBALL61

# マウスボタンを有効化（QMK 0.30で必須）
MOUSEKEY_ENABLE = yes

OLED_ENABLE = yes

RGB_MATRIX_ENABLE = no
# LED_VERSION=yes でビルドするとLED有効・メディアキー有効・マクロ無効の構成になる
# 通常版（指定なし）はメディアキー有効・LED無効・マクロ有効
ifeq ($(strip $(LED_VERSION)),yes)
    RGBLIGHT_ENABLE = yes
    EXTRAKEY_ENABLE = yes
    OPT_DEFS += -DLED_VERSION_BUILD
else
    RGBLIGHT_ENABLE = no
    EXTRAKEY_ENABLE = yes
    OPT_DEFS += -DGESTURE_ENABLE
endif

# タップダンス（フラッシュ節約のため無効化）
# TAP_DANCE_ENABLE = yes

# Auto Shift（フラッシュ節約のため無効化）
# AUTO_SHIFT_ENABLE = yes

# HIDハンドラ・タップダンス設定・詳細設定をビルドに含める
SRC += lib/keyball/kb_hid.c
SRC += lib/keyball/kb_settings.c
ifneq ($(strip $(LED_VERSION)),yes)
    SRC += lib/keyball/kb_macro.c
endif
