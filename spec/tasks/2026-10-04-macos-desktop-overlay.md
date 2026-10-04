---
task: macos-desktop-overlay
project: Tl
kind: implementation
created: 2026-10-04
memory_links:
  - spec/feature/desktop-overlay.md
  - spec/macos-composition.md
---

# macOS デスクトップ重ね表示と画面取り込みからの除外

## 背景

Odeum (旧 Spectator、Volputas の相棒) に GLab 向けのライブプレゼン配信を足す (neco 2026-10-04)。発表者アプリは画面を取り込んで配信し、視聴者のグッド・スタンプ・コメント・投票の集計を **Tela/Pictor で発表者のデスクトップに重ねて描く**。Mac 対応が必須。設計の全体は Castra `E:/Document/Ars/spec/plan/2026-10-04-odeum-live-presentation-design.md`。

いまの Tela は、デスクトップに単独で浮かぶ面 (`SPEC-TL-DESKTOP-OVERLAY`) を Windows (`tela::WindowsDesktopOverlay`) でしか持たない。macOS には対象ウインドウに追従する `MacOSOverlay` しかない。

## 価値とドメイン (Tela は DDD 必須)

- 価値: `spec/ux/product.md` の UX-TL-W7 (デスクトップに浮かぶ面)。失うと困る状態 = 「Mac の発表者が、配信中の画面を邪魔されずに視聴者の反応を隅で見続けられる」。UX-TL-W7 の記述が Windows 前提なら macOS と取り込み除外を追記する。
- ドメイン: `desktop-overlay` (specRefs `SPEC-TL-DESKTOP-OVERLAY`)。新規の src とテストを実装より先に membership へ宣言する。方針の正本は `spec/architecture/ddd.md`。
- PR 本文に、属するドメイン名・価値 ID・specRefs・責務・実施/未実施の検証を書く。

## やること

1. **`tela::MacOSDesktopOverlay` を足す** (`src/infrastructure/macos/`、公開ヘッダは既存 macOS ヘッダと同じ置き方)。`spec/feature/desktop-overlay.md` の契約を macOS で満たす。
   - 対象ウインドウを取らない、常に最前面、フォーカスを奪わない (nonactivating `NSPanel`)、Dock / Cmd+Tab / ミッションコントロールの切替対象に出ない、全スペースに出る。
   - 既定はクリック透過、宣言で exclusive とした領域だけ入力を受ける (既存 `input_panel` / `region_surface` の部品を再利用する。同じ仕組みを 2 つ作らない)。
   - 置き場所: メインディスプレイの作業領域 (`visibleFrame`) の四隅 + 余白、または絶対座標。大きさと余白は論理ポイント、ディスプレイの倍率 (`backingScaleFactor`) 変化に追従。
   - `grip` で指定した排他要素をつまみにして移動でき、移動後に `moved` コールバックを呼ぶ (保存は呼び出し側)。
   - `DesktopOverlayOptions` / `DesktopPlacement` / `DesktopCorner` は Windows 版と共通の型を使う。Windows 専用の型になっていれば OS 非依存の場所へ移す (Windows 版の動きは変えない)。
   - AppKit メインスレッド制約と寿命 (Runtime / PictorSurface が長生き) は `spec/macos-composition.md` に合わせる。Core に Objective-C / AppKit を持ち込まない。
2. **画面取り込みからの除外オプション** `DesktopOverlayOptions::exclude_from_capture` (既定 false) を足す。
   - macOS: `NSWindow.sharingType = NSWindowSharingNone` を見た目用・入力用の全ウインドウに設定。
   - Windows: `SetWindowDisplayAffinity(hwnd, WDA_EXCLUDEFROMCAPTURE)` を全ウインドウに設定 (失敗時は Windows 10 2004 未満として `WDA_MONITOR` へは落とさず、失敗を呼び出し側へ返す)。
   - 発表者アプリが自分の重ね表示を配信映像に映さないために使う。
3. 試用例 `examples/macos/desktop_overlay_probe.mm` (`--font`, `--corner`, `--seconds 1..3600`, `--exclude-from-capture`) と CMake ターゲット `tela_macos_desktop_overlay_probe`。
4. 仕様: `spec/feature/desktop-overlay.md` に macOS 節と `exclude_from_capture` を追記、`spec/contracts.md` の SPEC-TL-DESKTOP-OVERLAY を OS 共通の契約に更新、`spec/macos-composition.md` から参照、README の Components 表を更新。
5. ドメイン: 新規ファイルを `spec/domains/desktop-overlay.domain.json` (または該当ドメイン) の membership に含める。
6. テスト: OS 非依存の部分 (置き場所の計算、隅・余白・倍率の換算、作業領域への収め方) を Core 側の関数に寄せて `tests/` に単体テストを足す。AppKit 部分は Mac 拠点でのビルド確認が必要。

## 受入条件

- Windows で既存ビルドとテストが壊れない (`-DTELA_BUILD_WINDOWS=ON` 構成を含む)。Windows の `exclude_from_capture` 実装が入っている。
- macOS 向けコードは `TELA_BUILD_MACOS=ON` でのみビルドされ、Windows 構成に影響しない。
- macOS のビルド・実機確認は Mac 拠点が必要。実行は依頼者の許可範囲に従い、未実施は PR に未実施と明記する。
- 最小版・スタブで済ませない。

## 作業の進め方

- この worktree で作業し、ブランチは現在のものを使う。`git add <パス>` + `git commit` は可 (パス指定で stage、`git add -A` は使わない)。
- Anatomia は `node E:/Document/Ars/Anatomia/bin/anatomia.mjs` で呼ぶ。
- local PR は Concordia の `/v1/prs/local/direct` で提出する。完了報告は delegation status へ送る。Actio の本文 API が取れなければこのファイルを本文として扱う。
