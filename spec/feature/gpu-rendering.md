# GPU rendering

UX-TL-W2/W4。ホストが詳細領域の切り替えを知らせ、GpuReveal が残り時間を所有する。
120 ms の間だけ指定した物理ピクセル領域に背景色を薄く重ねる。操作とヒット領域は常に最新の状態を使う。
連続操作では最新の遷移に置き換え、動きを減らす指定では即時に終端へ進む。完了後の追加フレームは不要。

SPEC-TL-GPU。Tela の配置結果を Pictor の Vulkan GPU 描画で描く (`Tela::Gpu`)。
[計画書](../plan/2026-11-23-web-renderer-and-studio.md) の GPU 描画の段。Actio: `actio:255cd33d-cf29-4291-b752-deb3ff255533`。
規則の正本は [contracts](../contracts.md) の SPEC-TL-GPU。

依頼 (2026-11-23, neco): 「Tela の描画を GPU に対応して」。Ergo Studio のネイティブの窓は今の Vulkan / Pictor の GPU 描画のまま
にし、CPU raster に落とさない。

## 形

```
Runtime ─ plan_gpu_frame ─> GpuFrame (矩形・切り抜き・レイヤ)  ← GPU を使わない。ctest で確かめる
                               │
GpuRenderer::prepare ── 変わったレイヤだけ CPU でラスタ化して転送 (Pictor の文字 + Tela の図形ラスタ)
GpuRenderer::record  ── Pictor UIRenderer に Rect / PushClip / Image を記録 (ホストの render pass の中)
```

- 窓・swapchain・フレームの回し方はホストが持つ。Tela が受け取るのは Pictor の `VulkanContext`、描き込む `VkRenderPass`、
  その色の形式 (`VkFormat`)、Pictor の UI シェーダ (`ui_widget.*.spv`) の場所だけ。GLFW の窓でも Ergo Studio の
  `ergo_render` の層でも同じ口で描ける。
- 部品の見た目は `control_visual` と要素の種類 (panel / button / text / canvas) で決まり、GPU バックエンドは描くだけ。
  配置・切り抜き・スクロールは CPU raster・Web と同じ Runtime の結果を使う。

## 文字をどう描くか (Pictor を調べて決めたこと)

Pictor の `UIRenderer` は角丸の矩形・テクスチャ画像・9-slice・scissor だけを描き、Text 命令は無視する
(`include/pictor/ui/ui_renderer.h`)。さらに次の 2 つの制約がある。

- 命令ごとの UV が無い (画像はテクスチャ全体を貼る) ので、1 枚のグリフアトラスから字を切り出して描けない。
- 登録できるテクスチャは 64 枚で、登録したものは外せない。文字列ごと・字ごとにテクスチャを作ることはできない。

Ergo Studio も同じ理由で、1 フレームの文字を窓の大きさの 1 枚のテクスチャ (TextCanvas) に CPU で描いてから、最後に重ねている。
Tela はこのやり方を基にしたが、最後に 1 枚だけ重ねると、文字の上に後から描く矩形 (キャレット、重なった要素) が文字の下に回ってしまう。
そこで次のようにした。

- 文字は CPU 版と同じ Pictor の TrueType ラスタライザ (`TextImageRenderer`、`PictorText`) で、窓と同じ大きさの**レイヤ**に描く。
  フォント・字形・位置は CPU 版と画素単位で同じになる。Pictor の GPU 文字描画 (`profiler/bitmap_text_renderer`) は
  固定のビットマップフォントで日本語を描けず、ブラウザ版・CPU 版と同じ文言を出せないので使わない。
- 楕円・線・グラデーション・折れ線・画像も UIRenderer では描けない。これらも CPU 版と同じ `paint_drawing` でレイヤに描く。
- 塗りの矩形 (角丸を含む) と円は GPU の矩形で描く。
- 描く順序は崩さない。レイヤの中身に後から重なる矩形が来たら、先にそのレイヤを合成する命令を置いて次のレイヤに移る。
  レイヤを使い切ったときは、重なる矩形を最後のレイヤに CPU で描き込む。こうすると、レイヤが何枚であっても順序は正確に保たれる
  (既定は 3 枚、上限 8 枚)。
- 変わったレイヤだけをラスタ化して転送する (前のフレームの計画と比べる)。カーソルの点滅のような変化は無い
  (Tela にはアニメーションが無い)。

## 色・アルファ・DPI

- 描き込み先が `*_SRGB` の形式なら、テーマの色 (sRGB) を線形にして矩形に渡し、レイヤのテクスチャも `R8G8B8A8_SRGB` にする
  (書き込み時に sRGB に戻る)。UNORM の形式ならそのまま渡す。
- UIRenderer は straight alpha で合成するので、CPU で描いたレイヤ (premultiplied BGRA) は転送時に straight RGBA に直す (`layer_texels`)。
- DPI: Runtime の viewport の `dpi_scale` を物理ピクセルへの倍率とする。矩形は CPU の塗りと同じ画素 (外側の画素の端まで) に、
  切り抜きは CPU の画素判定と同じ画素 (左上の角が切り抜きの中にある画素) に揃える。整数の DPI では CPU 版と画素単位で一致する。
  小数の DPI では、合成の色空間 (sRGB 対応の書き込み先は線形で合成) と角丸の縁のぼかしが CPU 版と少し違う。

## 変わったときだけ描く

`GpuRenderer::prepare` は Runtime の配置結果から計画を作り、変わったレイヤだけを転送する。いつ描くかはホストが決める。
`Runtime::needs_frame()` が真のときか、swapchain を作り直したときだけ `prepare` → `record` → present を行い、そのあと
`frame_presented()` を呼ぶ。確認用のホストは、入力が無い間は `glfwWaitEventsTimeout` で眠っている。

## 使い方

```cpp
pictor::VulkanContext& vk = ...;   // ホストの窓と swapchain
tela::GpuRenderer renderer({vk, render_pass, vk.swapchain_format(), shader_dir}, "font.ttf");
runtime.text_measure(renderer.measure());     // クリックしたところにキャレットが来るように
// フレームごと (needs_frame のとき)
renderer.prepare(runtime);                    // render pass の外
vkCmdBeginRenderPass(cmd, ...);
renderer.record(cmd, extent);                 // render pass の中
vkCmdEndRenderPass(cmd);
// submit / present のあと
runtime.frame_presented();
```

確認用ホスト `tela_gpu_controls --font <file.ttf> [--shaders <dir>] [--seconds N]`: GLFW + Vulkan の窓に、段 1 の部品を
すべて並べた見本 (`ControlsGallery`。`tela_controls` と `tela_web_stdio` が使うものと同じ宣言) を GPU で描く。
マウス (左ボタン・移動・ホイール)、キー (Tab・Enter・矢印など)、確定した文字 (GLFW の char。IME の確定文字を含む) を Runtime に渡す。
窓の大きさ・content scale (DPI)・最小化・フォーカスは viewport に反映する。

ビルド: `-DTELA_BUILD_GPU=ON -DCMAKE_PREFIX_PATH=<Pictor の SDK>` (Pictor は `PICTOR_HAS_VULKAN` と GLFW 付きでビルドしたもの。
`pictor_core` があれば一緒にリンクする)。`-DTELA_PICTOR_SHADER_SOURCE_DIR=<Pictor>/shaders/ui` を付けると、Vulkan SDK の
glslc で `ui_widget.vert/.frag` をビルドディレクトリの `shaders/` にコンパイルし、確認ホストはそこを既定の場所として使う。

## 実装

| ファイル | 責務 |
|---|---|
| include/tela/gpu_frame.hpp, src/infrastructure/gpu/gpu_frame.cpp | 配置結果 → GPU を使わない描画計画 (矩形・切り抜き・レイヤの中身) |
| src/infrastructure/gpu/gpu_layering.cpp | 矩形とレイヤの振り分けと、描く順序の保持 |
| src/infrastructure/gpu/layer_texels.cpp | premultiplied BGRA → straight RGBA (テクスチャの行幅に合わせる) |
| src/infrastructure/gpu/layer_raster.cpp | レイヤの中身を Pictor の文字と Tela の図形ラスタで描く |
| src/infrastructure/gpu/vulkan_layers.cpp | レイヤのテクスチャ・staging・転送 |
| src/infrastructure/gpu/ui_commands.cpp | 計画 → Pictor `UIDrawCmd` (sRGB の線形化・scissor) |
| include/tela/gpu_renderer.hpp, src/infrastructure/gpu/gpu_renderer.cpp | 公開の口 `GpuRenderer` (準備・記録・文字の幅) |
| src/infrastructure/pictor/pictor_text.cpp | フォントの読み込みと折り返すラベル (`draw_box`) を CPU 版と共有 |
| examples/gpu/gpu_controls_host.cpp, examples/gpu/glfw_input.cpp | 確認用ホスト `tela_gpu_controls` |
| tests/gpu_frame_test.cpp | GPU を使わない部分の契約テスト (CTest `tela_gpu_frame_contract`) |

`Tela::GpuFrame` は Pictor も Vulkan も要らないので、常にビルドしてテストする。`Tela::Gpu` と確認ホストは `TELA_BUILD_GPU` のときだけ作る。

## 検証

- 2026-11-23: MSVC Release で `TELA_BUILD_GPU=ON TELA_BUILD_WINDOWS=ON` (Pictor `build/avatar-live-sdk`、Vulkan SDK 1.4.341.1)
  の全ターゲットがビルドでき、警告もエラーも無かった。シェーダは glslc で spv にした。標準の構成 (`scripts/verify.ps1` と同じ
  build-review) でも `tela_gpu_frame_tests` をビルドして実行し、通った。
- **ctest の一括実行はしていない** (この環境で ctest の実行が権限で止められた)。`scripts/verify.ps1` の所要時間も測っていない。
- **窓での実描画・入力は確かめていない** (`tela_gpu_controls` の起動・GPU での描画・マウス・キー・IME・DPI の切り替え・
  窓の大きさの変更)。Vulkan のデバイスを使う確認は、この委託では行っていない。

## 未確認・残り

- Ergo Studio への組み込み (段 3) は Ergo 側の作業。Studio は `ergo_render` の層 (`IRenderLayer`) から `prepare` (update) と
  `record` を呼ぶ形になる。
- 小数の DPI での CPU 版との画素の違いは測っていない。
