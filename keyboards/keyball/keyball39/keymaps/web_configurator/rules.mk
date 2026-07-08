# Web Configurator keymap for Keyball39
# WebHID通信を有効化
RAW_ENABLE = yes

# Viaのraw_hid_receiveが独自プロトコルと競合するため無効化
VIA_ENABLE = no

# EEPROMへのキーマップ保存を有効化
DYNAMIC_KEYMAP_ENABLE = yes

# マウスボタン（MS_BTN1-5はPointing Device経由で動作するため不要）
MOUSEKEY_ENABLE = no

OLED_ENABLE = yes

RGB_MATRIX_ENABLE = no
# LED_VERSION=yes でビルドするとLED有効・メディアキー無効・一部機能削減版になる
# 通常版（指定なし）はメディアキー有効・LED無効
ifeq ($(strip $(LED_VERSION)),yes)
    RGBLIGHT_ENABLE = yes
    EXTRAKEY_ENABLE = no
    OPT_DEFS += -DLED_VERSION_BUILD
else
    RGBLIGHT_ENABLE = no
    OPT_DEFS += -DGESTURE_ENABLE
endif

# タップダンス（フラッシュ節約のため無効化）
# TAP_DANCE_ENABLE = yes

# Auto Shift（フラッシュ節約のため無効化）
# AUTO_SHIFT_ENABLE = yes

# HIDハンドラ・詳細設定・マクロをビルドに含める
SRC += lib/keyball/kb_hid.c
SRC += lib/keyball/kb_settings.c
SRC += lib/keyball/kb_macro.c
