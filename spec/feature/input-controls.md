# Input controls

SPEC-TL-CONTROLS。Ergo Studio の UI を Tela で描き直す ([計画書](../plan/2026-11-23-web-renderer-and-studio.md) の段 1) ために、
Studio の `ViewBuilder` が使う部品と文字入力を Tela の宣言と Runtime に足す。Actio: `actio:3065429c-591a-4016-b247-38ff0f5231cf`。
規則の正本は [contracts](../contracts.md) の SPEC-TL-CONTROLS。

依頼 (2026-11-23, neco): 「ErgoStudio を Web 上から色々操作できるようにしたい」「Ergo だけ別の仕組みなのはなんか嫌だな。Tela で実装し直す」。

## 部品

| 宣言 | 中身 | 入力 |
|---|---|---|
| `text_field(id, value, {placeholder, secret, on_change, on_submit})` | 1 行。空のときは placeholder を薄く出す。secret は 1 文字 1 個の `•` | exclusive |
| `text_area(id, value, on_change, {.lines = 行数})` | 複数行。`\n` でだけ折る (自動の折り返しはしない) | exclusive |
| `slider(id, fraction, on_set)` | 0..1。押した位置へ動き、ドラッグで追う | exclusive |
| `list_item(id, label, detail, selected, action)` | 選択中は帯と左の印。detail は右寄せで薄く | exclusive |
| `separator(id)` | 横線 1 本 | passthrough |
| `status(id, label, tone)` | 良否の丸 (neutral / good / warning / bad) と文言 | passthrough |
| `scroll(id, children, {.height})` | 中身を切り抜いて縦にスクロール。高さ 0 は親の残りを埋める | exclusive |
| `video(id, label, stream)` | ブラウザでは渡された MediaStream を重ねる。ネイティブは枠と文言 ([Web 描画](web-rendering.md)) | passthrough |

## 値と表示の状態

- 値の持ち主はアプリ。入力欄は `on_change(新しい値)`、Enter で `on_submit()`、スライダーは `on_set(fraction)` で知らせるだけで、
  Runtime は値を書き換えない。アプリはコールバックの中で宣言し直す (見本: `examples/controls/controls_gallery.cpp`)。
- Runtime が ID ごとに持つのはキャレット・選択の起点・スクロール位置だけ。宣言を作り直しても残り、値が短くなればその中へ寄せる。
- フォーカスは Runtime が持つ。入力欄・スライダーを押すとフォーカス、それ以外を押すと外れる、Tab / Shift+Tab で並び順に移る、
  Escape で外れる。スクロールの中で隠れている部品にフォーカスが移ると見える位置までスクロールする。

## 入力の取り決め

- `HostKeyEvent` (キー・押下・修飾キー) と `HostTextEvent` (確定した文字列。IME の確定文字もここ)。変換中の文字は送らない。
- 編集キー: 左右 (Shift で選択)、Home / End (Ctrl で全体の端)、上下・PageUp / PageDown (text_area)、Backspace / Delete、
  Ctrl+A (全選択)、Enter (text_field は submit、text_area は改行)。スライダーは矢印 ±0.05、Page ±0.25、Home / End。
- ポインタの `wheel` は下にある一番内側の scroll を 1 目盛り 3 行動かす。端に着いたら外側の scroll へ渡す。
- Windows の窓 (`WindowsView`) は WM_KEYDOWN / WM_CHAR / WM_MOUSEWHEEL を渡す。Escape はフォーカスが無いときだけ窓を閉じる。
  クリックの位置からキャレットを決める文字幅は `PictorSurface::measure()` (描くときと同じ字送り)。

## 見た目の分け方

部品の見た目は配置結果 (`PlacedElement`・状態・テーマ) だけから `control_visual` が決める (矩形・楕円・1 行の文字、論理ピクセル、
描く順)。CPU raster (`control_paint`) はそれを描くだけで、GPU の描画を足すときも同じ並びを描けばよい
(2026-11-23 親セッション経由の人間の決定: GPU 描画は段 1・段 2 の後の別の段)。
入力欄の表示規則 (`field_view`: 折り返しなし、キャレットが見えるだけ横にずらす、キャレットの行が見えるように先頭行を決める) は
`web/tela.js` も同じ規則をブラウザのフォントで適用する。

## 実装

| ファイル | 責務 |
|---|---|
| include/tela/document.hpp, src/domain/composition/document_controls.cpp | 部品の宣言と上限 |
| include/tela/text_edit.hpp, src/domain/text/text_edit.cpp | UTF-8 の境界・行・挿入・削除・入力の整え (純粋関数) |
| include/tela/field_geometry.hpp, src/domain/composition/field_geometry.cpp | 入力欄の表示規則・クリック位置→キャレット・スライダー位置 |
| include/tela/control_visual.hpp, src/domain/composition/control_visual.cpp | 描画手段に依存しない部品の見た目 |
| src/domain/composition/layout.cpp | scroll の配置・切り抜き・位置の上限、separator の高さ |
| src/application/runtime_pointer.cpp | クリック・ドラッグ (スライダー・選択)・ホイール |
| src/application/runtime_keyboard.cpp | フォーカス・Tab・編集キー・確定文字 |
| src/infrastructure/pictor/control_paint.cpp, pictor_text.cpp | CPU raster で見た目の並びを描く、1 行の文字と字送り |
| src/infrastructure/windows/native_keyboard.cpp, native_pointer.cpp | Windows のキー・文字 (サロゲート対) ・ホイール |
| examples/controls/controls_gallery.*, examples/windows/controls_host.cpp | 全部品の見本 (`tela_controls --font <ttf>`) |
| tests/controls_test.cpp, tests/text_edit_test.cpp | 契約テスト (CTest `tela_controls_contract`, `tela_text_edit_contract`) |

## 検証

- 2026-11-23: MSVC Release (`TELA_BUILD_WINDOWS=ON`、Pictor 込み) で全ターゲットのビルドが警告・エラーなしを確認。
- **ctest は実行していない** (この環境で実行の権限が通らなかった)。既存のテストを含め、実行は PR の確認で行う。
- `tela_controls` の窓での実際の見え方・キー入力・IME の確定・ホイールは未確認。

## 未確認・残り

- クリップボード (Ctrl+C / Ctrl+V) はネイティブの窓では扱わない。ブラウザでは貼り付けが確定文字として届く。
- 単語単位の移動・ダブルクリック選択・undo は無い。
- Unity の Scene view に重ねる窓 (overlay) はキー入力を受けない (ホイールとクリックの部品は動く)。
