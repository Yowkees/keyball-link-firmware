# Keyball Link ファームウェア（AVR版） — 引き継ぎメモ

このリポジトリにはこれまで引き継ぎメモが無かった。今はデバイス間の作業共有の説明だけを置く。プロジェクト全体の詳細は[keyball-link/HANDOFF.md](https://github.com/Yowkees/keyball-link)を参照。

## デバイス間の作業共有（2026-11-05導入、mac-desktopは2026-11-06導入）

2台のMac（`mac-note`＝サブ機、`mac-desktop`＝メイン機）の間で「何を作業中か」を見えるようにするため、3種類のブランチを使い分ける。

| ブランチ | 役割 | 更新方法 |
|---|---|---|
| `work/<デバイス名>`（`work/mac-note` / `work/mac-desktop`） | 各デバイスの「今の作業状態」をそのまま流す場所。毎日上書きされる | **自動**。mac-noteは毎日8:15、mac-desktopは毎日17:15にlaunchdで自動push |
| `main` | 公開用（実際にリリース済みのもの） | 動作確認が取れたものだけ、**手動**でmainへマージ・コミットする |
| `archive/<名前>` | 没案・一旦止めたプロジェクトの固定保存場所 | 一度作ったら自動pushの対象にしない。何かを保留する時に作り、下の索引に追記する |

**しくみの要点**:
- 自動pushは`git stash create`で今の作業状態のスナップショットを作り、`git push origin +<スナップショット>:refs/heads/work/<デバイス名>`で送るだけ。**ローカルの作業ブランチやファイルには一切手を加えない**。未コミットの変更が無い時は、今のHEADをそのまま送る。
- 送られるのは、**その時チェックアウトしているブランチ**の状態。
- **`git add`していない新規ファイル（未追跡ファイル）はスナップショットに含まれない**。新しいファイルを作ったら`git add`しておくと、次の自動pushに含まれる。
- 各デバイスの設定（スクリプトはローカルにだけ存在）:

| デバイス | スクリプト | launchdの設定 | ログ |
|---|---|---|---|
| mac-desktop | `~/Scripts/keyball-work-push.sh` | `~/Library/LaunchAgents/com.ineno.keyball-work-push.plist` | `~/Library/Logs/keyball-work-push.log` |
| mac-note | `~/Scripts/keyball-work-push.sh` | `~/Library/LaunchAgents/com.inenoyuki.keyball-work-push.plist` | `~/Library/Logs/keyball-work-push.log` と `keyball-work-push.launchd.log` |

- 失敗した時は、まずログを見る。手動で動かす時は`~/Scripts/keyball-work-push.sh`を実行する。

**保留中プロジェクト索引（archive/*）**: 現時点でなし。何か保留したら、ブランチ名と一言メモをここに追記する。
