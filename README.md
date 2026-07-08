# Keyball Link Firmware

[Keyball Link](https://github.com/Yowkees/keyball-link)（Keyball用Webキーマップ設定ツール）専用ファームウェアのソースコードです。
Keyball39 / 44 / 61 に対応しています。

[QMK Firmware](https://github.com/qmk/qmk_firmware) と [Yowkees/keyball](https://github.com/Yowkees/keyball) をベースに、
Webブラウザ（WebHID）からキーマップ・各種設定を書き換えられるよう拡張したものです。

## 構成

| パス | 内容 |
|---|---|
| `keyboards/keyball/` | keyball一式（`keymaps/web_configurator` が本ファームのキーマップ） |
| `patches/0001-usb-descriptor-jis-country-code.patch` | QMK本体修正: USBディスクリプタの国コードをJIS(15)に設定（macOSでJIS配列と認識させる） |
| `patches/0002-auto-mouse-runtime-threshold.patch` | QMK本体修正: 自動マウスレイヤーのしきい値を実行時に変更可能にする |

## ビルド方法

ベースにした QMK Firmware のコミット: `594558ec7b9ac1963870447778426682065e0d20`

```bash
# 1. QMK Firmware を取得してベースコミットに合わせる
git clone https://github.com/qmk/qmk_firmware.git
cd qmk_firmware
git checkout 594558ec7b9ac1963870447778426682065e0d20
make git-submodule

# 2. QMK本体へのパッチを適用
git apply /path/to/keyball-link-firmware/patches/*.patch

# 3. keyball一式を配置
cp -R /path/to/keyball-link-firmware/keyboards/keyball keyboards/

# 4. ビルド
qmk compile -kb keyball/keyball39 -km web_configurator                    # 通常版
qmk compile -kb keyball/keyball39 -km web_configurator -e LED_VERSION=yes # LED版
# keyball44 / keyball61 も同様
```

## 2バージョンについて

Pro Micro（ATmega32U4）のフラッシュ容量の都合で、全機能＋LEDは同時に収まらないため2版構成です。

| バージョン | 内容 |
|---|---|
| 通常版 | メディアキー・マクロ・ジェスチャー・自動マウスレイヤー・キー単位詳細設定など全機能（LEDなし） |
| LED版（`LED_VERSION=yes`） | LED点灯＋マクロ・自動マウスレイヤー・基本設定のみの最小構成 |

## ライセンス

GPL-2.0（[LICENSE](LICENSE)）。QMK Firmware および Yowkees/keyball 由来のコードを含みます。
