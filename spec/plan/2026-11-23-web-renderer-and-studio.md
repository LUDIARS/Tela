# Tela の Web 描画と、Ergo Studio を Tela で描く計画

依頼 (2026-11-23, neco):
- 「ErgoStudio を Web 上から色々操作できるようにしたい。ErgoStudio のウインドウ、UI を Web 経由で描画する」
- 「Tela を Web レンダリングする実装を作る」
- 「Ergo だけ別の仕組みなのはなんか嫌だな。Tela で実装し直す」

つまり、Ergo Studio の UI を Tela で作り直し、同じ UI をネイティブの窓とブラウザの両方に Tela が描く。
ブラウザ版は窓の映像ではなく、Tela の配置結果をブラウザが描く (文字はブラウザのベクタフォント)。

## 現状

| | Tela | Ergo Studio |
|---|---|---|
| 宣言 | `Document` (panel / text / button / canvas) | `ViewBuilder` (title, label, button, field, text_field, secret_field, text_area, list_item, slider, wrapped, clipped_label, status, separator, spacer, row, panel) が `ergo_ui_kit` の木を作る。全 15 のタブがこれだけを使う |
| 配置・入力 | `Runtime` (配置・切り抜き・押下状態・ポインタ) | `StudioApp` (タブ・スクロール・フォーカス・クリック・スライダー・文字入力) |
| 描画 | CPU raster (Pictor のテキスト) → Windows / macOS の窓 | GLFW + Vulkan + Pictor UIRenderer + フォントアトラス |
| ブラウザ | なし | 別に作った HTML のページ (web プラグイン) |

Tela に無いもの: 文字入力 (キーボード・テキスト・フォーカス・キャレット)、入力欄、スライダー、一覧の行、スクロール、Web 描画。

## 段階 (1 段 = 1 PR。各段はその範囲をすべて作り、配線し、試す)

### 段 1 (Tela): Studio に要る部品と文字入力

- 宣言に足す: `text_field` (1 行・伏せ字)、`text_area` (複数行)、`slider` (0..1)、`list_item` (選択中の印・補足)、
  `separator`、`status` (良否の印)、`scroll` (中身を切り抜いて縦にスクロールする入れ物)。
- 入力の取り決めに足す: `HostKeyEvent` (キー・押下・修飾キー)、`HostTextEvent` (確定した文字列)。
  ポインタの `wheel` をスクロールに使う。フォーカスは Runtime が持つ (クリックした入力欄、Tab で次へ)。
- 値の持ち主はアプリ: 入力欄は `on_change(value)` / `on_submit()`、スライダーは `on_set(fraction)` で知らせる。
  Runtime はキャレット・選択・スクロール位置だけを持つ (ID ごと、宣言を作り直しても残る)。
- CPU raster と Windows の窓の両方で描け、入力が通ること。

### 段 2 (Tela): Web 描画 `Tela::Web`

- `WebSession` (通信路を持たない): Runtime の配置結果を「フレーム」(版番号・テーマ・要素の種類と位置・切り抜き・文字・
  図形・状態) の JSON にし、ブラウザからの入力 JSON (pointer / key / text / resize) を Runtime に渡す。
  変わったときだけフレームを出す。上限 (フレームの大きさ、要素数、入力の長さ) を置く。
- ブラウザ側の描画スクリプト `web/tela.js` (canvas 2D、外部依存なし): 図形・グラデーション・画像・文字 (ブラウザのフォント)・
  キャレットと選択・切り抜き・スクロール。ポインタ・キーボード・IME の確定文字・大きさの変更を送る。
- 通信は組み込む側 (Ergo Studio の WS など) が運ぶ。Tela は HTTP サーバーを持たない。
- `video` 要素: ブラウザでは指定した MediaStream を要素の位置に重ねる (Ergo の遠隔プレイの映像)。ネイティブでは枠と文言だけ。

### 段 3 (Ergo): Studio の UI を Tela に載せ替える

- `ViewBuilder` の公開の形はそのままに、中身を `tela::Document` を作る形にする (15 のタブは原則そのまま)。
- `StudioApp` のタブ・スクロール・フォーカス・入力を Tela の Runtime に任せる。
- ネイティブの窓を Tela の窓 (`Tela::Windows` の view host) にする。
- 同じ UI を `Tela::Web` で Studio の WS から配る。公開名 (Cloudflare Access) の検査は今の request_guard のまま。
- HTML のページにしか無いもの (アセット一覧・遠隔プレイの映像と操作) を Studio のタブにする。

### 段 4 (Ergo): 古い描画の撤去

- web プラグインの HTML のページ、Studio の GLFW + Vulkan の UI 層とフォントアトラスを消す (Tela に移ったことを確かめてから)。

## 決め事

- Tela は Ergo に依存しない (Ergo が Tela を使う)。Pictor は Tela に依存しない (従来どおり)。
- 値の正本はアプリ (Studio のプラグイン)。Tela はキャレット・スクロール・押下などの表示の状態だけを持つ。
- ブラウザに送るのは配置結果だけで、アプリのデータや秘密 (伏せ字欄の中身) は送らない (伏せ字は `•` に置き換えて送る)。
- Web とネイティブで同じ宣言・同じ配置を使う (別の UI を作らない)。

### GPU 描画の段 (Tela)

人間の指示 (2026-11-23, neco): 「Tela の描画を GPU に対応して」。Studio のネイティブの窓は今の Vulkan / Pictor の GPU 描画のまま
にし、CPU raster に落とさない。下の未確定だった問いへの答えになる。

- `Tela::Gpu` `GpuRenderer`: 配置結果を Pictor の UIRenderer で、ホストが持つ render pass に描く ([GPU rendering](../feature/gpu-rendering.md))。
- 段 3 では、Studio の窓を Tela の CPU raster の窓に替えず、`ergo_render` の層から `GpuRenderer` を呼ぶ。

## 決まったこと (元は未確定)

- 段 3 で Studio のネイティブの窓の描画を Vulkan (Pictor の GPU) から Tela の CPU raster に替えてよいか →
  替えない。GPU のまま描く (上の GPU 描画の段)。