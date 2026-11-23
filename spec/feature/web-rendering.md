# Web rendering

SPEC-TL-WEB。同じ Tela の宣言と配置を、ネイティブの窓に加えてブラウザにも描く (`Tela::Web`)。
[計画書](../plan/2026-11-23-web-renderer-and-studio.md) の段 2。Actio: `actio:3065429c-591a-4016-b247-38ff0f5231cf`。
規則の正本は [contracts](../contracts.md) の SPEC-TL-WEB。

依頼 (2026-11-23, neco): 「ErgoStudio のウインドウ、UI を Web 経由で描画する」「Tela を Web レンダリングする実装を作る」。

## 形

```
アプリ (宣言) ─ Runtime ─ WebSession ──文字列──> 組み込み側の通信路 (WS など) ──> web/tela.js (canvas 2D)
                         <──入力 JSON── ─────────────────────────────────── <── ポインタ・キー・確定文字・大きさ
```

- 窓の映像ではなく配置結果を送り、文字はブラウザのフォントで描く。
- Tela は HTTP サーバーも WebSocket も持たない。誰を繋ぐか (認証・公開名の検査) は組み込み側が決める。
- 1 つの WebSession は 1 つの Runtime を 1 つのページに出す。ネイティブの窓と同時に出すときは Runtime を分ける
  (Runtime の viewport は 1 つ)。

## 使い方

```cpp
tela::Runtime runtime;          // アプリが宣言する
tela::WebSession session(runtime);
// ページからの文字列
if (!session.receive(message)) log(session.last_error());
// 送るもの (画像 → フレームの順)。変わっていなければ空
for (auto& out : session.poll()) socket.send(out);
// ページが閉じた
session.disconnect();
```

```html
<div style="position:relative"><canvas id="ui" style="width:100%;height:100%"></canvas></div>
<script src="tela.js"></script>
<script>
  const view = Tela.attach(document.getElementById('ui'), { send: (m) => socket.send(m) });
  socket.onmessage = (e) => view.receive(e.data);
  view.attachStream('remote-play', mediaStream); // video 要素の位置に重ねる
</script>
```

確認用のホスト: `tela_web_stdio` は全部品の見本を宣言し、標準入力の 1 行を入力 JSON として受け、出す文字列を 1 行ずつ標準出力へ書く。

```
{"type":"resize","width":480,"height":640,"dpi":1}
{"type":"pointer","phase":"down","button":"primary","x":40,"y":60}
{"type":"text","text":"日本"}
```

## メッセージ

- 入力: `pointer` / `key` / `text` / `resize` / `focus`。座標は canvas の CSS ピクセル (= Tela の論理ピクセル)。
- 出力: `image` (canvas の画像。画像オブジェクトごとに 1 回) と `frame` (版 1・増える revision・テーマ・フォーカス・要素)。
- 上限: 入力 16 KiB・確定文字 4 KiB・JSON の深さ 4 / 値 64 個、フレーム 2 MiB・要素 4096、画像 1 MiB。
  フレームに入りきらない要素は送らず `truncated: true` を付ける。
- 伏せ字の欄は `•` だけを送り、キャレットは文字数で送る。中身のバイトはプロセスの外へ出ない。

## 描画手段との分け方

部品の見た目は配置結果から決まる ([input controls](input-controls.md) の `control_visual`)。ブラウザはフォントが違うため
見た目の並びそのものではなく配置結果と状態を受け取り、`tela.js` が同じ規則 (入力欄の横ずらし・先頭行、スライダー・一覧・状態・
スクロールの形) を自分のフォントで当てる。位置は配置で一致し、字形の細部は一致しない。

## 実装

| ファイル | 責務 |
|---|---|
| include/tela/web_session.hpp, src/application/web_session.cpp | 入力を Runtime へ、変わったときだけフレームを出す |
| src/infrastructure/web/input_decoder.cpp | 入力 JSON の解釈と上限 |
| src/infrastructure/web/json_reader.cpp, json_writer.cpp | 上限付きの JSON 読み書き |
| src/infrastructure/web/frame_encoder.cpp | 配置結果 → フレーム JSON (伏せ字・上限・切り捨て) |
| src/infrastructure/web/image_registry.cpp | 画像の ID と送信 (straight RGBA base64) |
| web/tela.js | canvas 2D の描画と入力の送信 (外部依存なし。install で `share/tela/web`) |
| examples/web/web_stdio_host.cpp | 確認用ホスト `tela_web_stdio` |
| tests/web_session_test.cpp | 契約テスト (CTest `tela_web_contract`) |

## 検証

- 2026-11-23: MSVC Release で `tela_web`・`tela_web_stdio`・`tela_web_tests` のビルドが警告・エラーなし。
  `node --check web/tela.js` で構文を確認。
- **ctest は実行していない** (この環境で実行の権限が通らなかった)。
- ブラウザでの実際の描画・IME・ホイール・MediaStream の重ね合わせは未確認 (ブラウザを使った確認をしていない)。

## 未確認・残り

- Studio の WS への組み込みと公開名の検査は段 3 (Ergo 側)。
- GPU 描画 (Vulkan / Pictor の GPU で配置結果を描く) は [GPU rendering](gpu-rendering.md) (SPEC-TL-GPU) で別に作った。
