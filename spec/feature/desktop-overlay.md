# Desktop overlay

SPEC-TL-DESKTOP-OVERLAY。対象ウインドウを持たず、デスクトップに単独で浮かぶ半透明の面を出す。
価値は UX-TL-W7（[product](../ux/product.md)）。2026-09-21 neco 承認。

## 目的

作業中ずっと見ておきたい小さな情報（部屋の状態、今日の予定など）を、ブラウザや専用の
ウインドウを開かずに画面の隅へ浮かべたい。これまでの面は、対象ウインドウに貼り付く
`WindowsOverlay` と、不透明な通常ウインドウ `WindowsView` の 2 つだけだった。どちらを使っても、
どのアプリにも属さず、デスクトップの隅に出しっぱなしにできる表示は作れない。

いちばん守ることは、下の作業を妨げないこと。クリックとキー入力を奪わず、最前面にあっても
下が透けて見え、下のウインドウをそのまま操作できるようにする。Lares のデスクトップ表示が
最初の利用先だが、Tela の汎用の面として足す。

## 使い方

```cpp
tela::Runtime runtime;
tela::PictorSurface renderer(font_path);          // TrueType。同梱しない
tela::DesktopOverlayOptions options;
options.placement.width = 360;                    // 論理ピクセル
options.placement.height = 216;
options.placement.corner = tela::DesktopCorner::bottom_right;
options.grip = "panel.grip";                      // つまみにする排他要素の ID
options.moved = [&](const tela::DesktopPlacement& p) { store(p); };  // 保存は呼び出し側
tela::WindowsDesktopOverlay overlay(runtime, renderer, std::move(options));
while(running) {
    // メッセージを配る → 宣言を更新 → synchronize() → MsgWaitForMultipleObjects で待つ
    runtime.document(declare(overlay.hovered()));
    overlay.synchronize();
}
```

別リポからは `find_package(Tela CONFIG REQUIRED)` と `Tela::Windows` で使う（`-DTELA_BUILD_WINDOWS=ON` で
ビルドした Tela を `cmake --install` した prefix を `CMAKE_PREFIX_PATH` か `Tela_DIR` で渡す）。
試すための例は `tela_desktop_overlay_probe --font <file.ttf> [--corner bottom-right] [--seconds 1..3600]`。

## 契約

規則は [contracts](../contracts.md) の SPEC-TL-DESKTOP-OVERLAY。

- 対象ウインドウを取らない。常に最前面にあり、フォーカスを奪わず、タスクバーにも Alt+Tab にも出ない
  （所有者なしのツールウインドウ、`WS_EX_NOACTIVATE`）。クリックされてもアクティブにならない。
- 半透明の出し方と入力の受け方は `WindowsOverlay` と同じ合成を使う。素通しの見た目用ウインドウ 1 枚と、
  排他領域ごとの入力用ウインドウ（その領域の形に切り抜く）で出す。両方を 2 つ作らないため、
  既存の実装から `LayeredComposition`（合成）と `NativePointer`（クリックを runtime の入力へ変える）を
  切り出し、`WindowsOverlay` / `WindowsView` もこれを使う。既存の面の動きは変えない。
- 既定では面全体が入力を素通しする。宣言で排他（exclusive）とした領域の上だけ入力を受ける。
  ボタンを押すと動作し、それ以外の場所のクリックは下のウインドウへ届く。
- 背景の半透明は、宣言側が自分の色で描く（例: canvas の角丸矩形をアルファつきの色で塗る）。
  面は渡された乗算済みビットマップをそのまま出すだけで、面そのものに透明度の設定は持たない。
  濃さは内容ごとに宣言で変えられる。この描き方は既存の Drawing で表せるので、Core と Pictor は変えていない。
- 置き場所と大きさは呼び出し側が渡す。
  - 隅: 主モニタの作業領域の四隅のどれかに、余白（論理ピクセル）をあけて置く。
  - 絶対座標: 左上をデスクトップの物理ピクセルで指定する。
  - 大きさと余白は論理ピクセルで、面が置かれるモニタの DPI で物理ピクセルに直す（Per-Monitor V2）。
    呼び出し側のプロセスの DPI 設定には依存しない。
- 絶対座標の面は、そのモニタの DPI で大きさを決めたときに作業領域に乗る割合がいちばん大きい
  モニタのものとし、そのモニタの作業領域の中へ寄せる。どのモニタにも乗らない（画面外）ときは、
  主モニタの同じ隅へ同じ余白で戻す。作業領域より大きい面は作業領域の大きさまで縮める。
- つまみ: `options.grip` に指名した要素の排他領域をドラッグすると、面ごと動く。つまみの入力は
  runtime へ渡さない（ボタンにしない）。離すと位置を作業領域の中へ寄せる。その位置に、いま
  いちばん近い隅とそこからの余白（論理ピクセル）を添えて `moved` に通知する。通知は `synchronize()` の
  中で行い、ウインドウメッセージの処理中には行わない。保存は呼び出し側の責務で、通知された値を
  置き場所として渡し戻すと同じ位置に出る。その位置のモニタが外れていれば、主モニタの同じ隅へ戻る。
- ドラッグ中は内容を出し直さない（ウインドウだけを動かす）。離した後の `synchronize()` で新しい
  位置に出し直す。つまみが離しを受け取れなかったときも、主ボタンが離れていればドラッグを終える。
- `synchronize()` は毎回モニタ構成と DPI を読み直す。置き場所が変わらなければビューポートを
  作り直さず、宣言が変わらなければ出し直さない。表示倍率の変更やモニタの抜き差しは次の
  `synchronize()` で反映する。読めるモニタが 1 つも無い間は隠し、戻れば出す。
- `hovered()` は、ポインタが乗っている排他領域の ID を返す（乗っていなければ空）。
  「つまみに乗ったら操作の段を開く」のような判断は宣言側で行う。
- 破棄すると、ウインドウ・キャプチャ・入力領域をすべて解放する。runtime から見ると、ホストを
  失ったときと同じように表示と入力の所有が解かれる。ビットマップと DC は提示のたびに解放し、
  入力領域の切り抜き（リージョン）は Windows に渡すか、渡せなかったときに削除する。

## 実装

| ファイル | 責務 |
|---|---|
| include/tela/desktop_placement.hpp | 隅・置き場所・モニタの値と配置の計算 API（Core、Windows 非依存） |
| src/domain/desktop/desktop_placement.cpp | 四隅と余白、DPI 換算、所属モニタの決定、画面外からの復帰、移動後の置き場所 |
| include/tela/windows_desktop_overlay.hpp | デスクトップに浮かぶ面の API |
| src/infrastructure/windows/windows_desktop_overlay.cpp | ウインドウの寿命、つまみのドラッグ、ポインタが乗っている領域、提示 |
| src/infrastructure/windows/desktop_monitors.* | モニタの作業領域と DPI の読み取り、Per-Monitor V2 への一時切替 |
| src/infrastructure/windows/layered_composition.* | 半透明の見た目用ウインドウと排他領域の入力用ウインドウ（WindowsOverlay と共用） |
| src/infrastructure/windows/native_pointer.* | 主ボタンと移動を runtime の入力へ変える（WindowsOverlay / WindowsView と共用） |
| examples/windows/desktop_overlay_probe.cpp ほか desktop_probe_* | 固定の宣言を半透明で浮かべる目視用の例 |
| tests/desktop_placement_test.cpp | 配置の契約テスト |
| tests/consumer | install した package から面をリンクする確認 |

## 検証

- `tela_desktop_placement_contract`（CTest）: 四隅と余白、DPI 換算、主モニタの判定、負の原点、
  端からはみ出した面の寄せ、2 つのモニタをまたぐ面の所属、画面外からの復帰、作業領域より大きい面、
  使えないモニタの除外と不正な値の拒否、移動後の置き場所とその往復。
- 見え方と実際の入力は自動では確かめられない。[windows-composition](../windows-composition.md) の
  「Desktop overlay acceptance」の手順で目で確かめる。
- 2026-09-21 に作業ブランチで行ったこと（例を 2 回起動し、どちらも 15 秒で自分で終了）:
  倍率 125% の主モニタで、作業領域の右下（右端 3840・下端 2100 から 20 物理ピクセル = 余白 16 論理）に
  450x270 物理ピクセル（360x216 論理）で出た。
  見た目用ウインドウは LAYERED・TRANSPARENT・NOACTIVATE・TOOLWINDOW・TOPMOST（APPWINDOW なし）。
  `WindowFromPoint` による当たり判定では、背景部分は下のデスクトップへ素通しし、ボタンとつまみは面に
  当たった。起動の前後でフォアグラウンドのウインドウは変わらなかった。画面の取り込みで、角丸の
  背景から下の壁紙が透け、文字の縁に不透明な四角が出ないことを見た。面の作成と破棄を 6 回繰り返しても、
  GDI オブジェクトは最初の GDI 使用で増えた 2 から増えず、USER オブジェクトは元の 1 に戻った。

## 未確認

実際にマウスで行うクリックとドラッグ（下のウインドウが入力を受け取ること、ボタンのクリック数、
つまみでの移動と通知）は未確認。DPI の異なるモニタへのドラッグ、表示倍率の変更、モニタを外したときの
主モニタへの復帰、タスクバーの位置の変更、フルスクリーンのアプリの上での見え方も実機では未確認で、
いずれも契約テストでしか確かめていない。キーボード・IME の入力は対象外（面はフォーカスを取らない）。
macOS 版は対象外。
