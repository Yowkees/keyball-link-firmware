# Changelog

Keyball Link ファームウェアの変更履歴です。バージョンは [Semantic Versioning](https://semver.org/lang/ja/) に従います。
Web版（keyball-link）とはバージョン番号は連動していません。Web版の変更履歴は [keyball-link](https://github.com/Yowkees/keyball-link/blob/main/CHANGELOG.md) を参照してください。

## [Unreleased]

## [1.1.0] - 2026-07-27
### Changed
- LED版のマクロ機能を廃止し、代わりにメディアキー（音量・再生停止など）を有効化
  （フラッシュ容量の都合で両方は同時に搭載できないため。通常版はこれまで通りメディアキー・マクロ両方が有効）

## [1.0.0] - 2026-07-22
### Added
- ファームウェアのバージョンをHID経由で問い合わせられるコマンド（GET_VERSION）を追加
### Fixed
- ジェスチャーを大きく振ると同じキーが複数回連続で送られてしまう不具合を修正（発火後にクールダウンを設け、1スイングにつき1回だけ発火するように）
- ジェスチャーの発動しきい値（横方向・縦方向）を個別にEEPROM保存できるように変更
