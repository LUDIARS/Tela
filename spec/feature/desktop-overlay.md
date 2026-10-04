# Desktop overlay

SPEC-TL-DESKTOP-OVERLAY。対象ウインドウを持たず、デスクトップに単独で浮かぶ半透明の面を出す。
価値は UX-TL-W7（[product](../ux/product.md)）。2026-09-21 neco 承認。
macOS 版と画面取り込みからの除外は 2026-10-04 neco 指示（Odeum のライブプレゼン配信で、Mac の発表者の
デスクトップに視聴者の反応を重ね、その重ね表示を配信映像に映さない）。

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

`DesktopOverlayOptions` は OS 共通の型（`include/tela/desktop_overlay_options.hpp`）で、macOS では
同じ値を `tela::MacOSDesktopOverlay` に渡す（下の「macOS」節）。配信する発表者アプリは
`options.exclude_from_capture = true` で、自分の重ね表示を画面の取り込み・録画・共有から外す。

別リポからは `find_package(Tela CONFIG REQUIRED)` と `Tela::Windows` で使う（`-DTELA_BUILD_WINDOWS=ON` で
ビルドした Tela を `cmake --install` した prefix を `CMAKE_PREFIX_PATH` か `Tela_DIR` で渡す）。
試すための例は `tela_desktop_overlay_probe --font <file.ttf> [--corner bottom-right] [--seconds 1..3600]
[--exclude-from-capture]`（macOS は `tela_macos_desktop_overlay_probe`、同じ引数）。

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
- 画面取り込みからの除外: `exclude_from_capture`（既定 false）を立てると、見た目用・入力用の
  すべてのウインドウを取り込みから外す。入力用のウインドウは排他領域の最終画素を持つので、
  1 枚でも漏れるとボタンだけが映る。Windows は作ったウインドウごとに
  `SetWindowDisplayAffinity(hwnd, WDA_EXCLUDEFROMCAPTURE)` を設定し、失敗した（Windows 10 2004 未満）
  ときはそのウインドウを壊して `std::runtime_error` を投げる。`WDA_MONITOR`（取り込み側で黒く塗る）には
  落とさない。黒い四角が配信に出るのは「映さない」の代わりにならないため。入力用ウインドウは
  `synchronize()` の中で作るので、その失敗は `synchronize()` から投げられる。
- 破棄すると、ウインドウ・キャプチャ・入力領域をすべて解放する。runtime から見ると、ホストを
  失ったときと同じように表示と入力の所有が解かれる。ビットマップと DC は提示のたびに解放し、
  入力領域の切り抜き（リージョン）は Windows に渡すか、渡せなかったときに削除する。

## macOS

`tela::MacOSDesktopOverlay(runtime, renderer, options)` が同じ契約を macOS で満たす。AppKit の寿命と
スレッドは [macos-composition](../macos-composition.md) に従う（生成・同期・非表示・破棄は
メインスレッド、NSApplication とイベントループは呼び出し側、Runtime と PictorSurface は面より長生き）。

```objc
[NSApp setActivationPolicy:NSApplicationActivationPolicyAccessory];  // Dock と Cmd+Tab に出さない
tela::DesktopOverlayOptions options;           // Windows と同じ型
options.placement.width = 360; options.placement.height = 216;   // 論理ポイント
options.exclude_from_capture = true;
tela::MacOSDesktopOverlay overlay(runtime, renderer, std::move(options));
// イベントを配る → 宣言を更新 → overlay.synchronize()
```

- 面は nonactivating `NSPanel`（`MacOSOverlay` と同じ `make_panel`）。`NSStatusWindowLevel` で他アプリの
  浮動パネルより前に出し、フォーカスは取らない。全スペースとフルスクリーンの横に出し
  （CanJoinAllSpaces・FullScreenAuxiliary）、ミッションコントロールの一覧とウインドウの切替
  （Stationary・IgnoresCycle）、Window メニューに出さない。Dock と Cmd+Tab はアプリ単位なので、
  呼び出し側が Accessory の activation policy を選ぶ（例はそうしている）。
- 見た目用パネルは全体をクリック透過にし、排他領域の断片ごとに入力用パネルを置く。この入力用パネルは
  `MacOSOverlay` と共用の `RegionPanels`（`input_panel` / `region_surface` を束ねたもの）で作り、
  同じ仕組みを 2 つ持たない。対象アプリが前面かどうかの判定は外し（面はどのアプリにも属さない）、
  つまみの入力は runtime の前で受け取る。
- 置き場所はポイントで解く。macOS には画面全体の物理ピクセル格子がないため、各画面の
  `visibleFrame`（メニューバーと Dock を除いた作業領域）を、主画面（メニューバーのある画面）の
  上端から下向きに測ったポイント座標の「モニタ」（倍率 1）にして `place_on_desktop` に渡す。
  大きさと余白は論理ポイントのまま、絶対座標もこのポイント座標で表す。面を抱えた画面の
  `backingScaleFactor` を Viewport の倍率とし、ポイント座標 × 倍率を Viewport の位置と画素数にする
  （`MacOSOverlay` と同じ座標基準）。この換算は Core の `place_on_point_desktop` /
  `point_placement_at`（`include/tela/desktop_points.hpp`）にあり、AppKit を含まない。
- `synchronize()` は毎回 `NSScreen.screens` を読み直す。倍率・作業領域・画面構成の変化は次の
  `synchronize()` で Viewport の revision を上げ、進行中の操作を取り消して入力用パネルを作り直し、
  新しい倍率で描き直す。画面が 1 つも無い間は隠す。
- つまみ: 指名した要素の入力用パネルで押すと、`NSEvent.mouseLocation` の差分（整数ポイント）だけ
  全パネルを動かす。離すと `point_placement_at` で位置を作業領域に寄せ、いちばん近い隅と余白
  （ポイント）を `synchronize()` の中で `moved` に通知する。離しを受け取れなかったときも
  `NSEvent.pressedMouseButtons` で主ボタンが上がっていればドラッグを終える。
- `hovered()` はポインタ位置にある表示中の入力用パネルの領域 ID を返す。`native_window()` は
  見た目用パネルの CGWindowID。
- `exclude_from_capture` は見た目用と入力用のすべてのパネルに `NSWindow.sharingType =
  NSWindowSharingNone` を設定する。
- 入力コールバックの例外は保存し、次の `synchronize()` で一度だけ投げて面を隠す。破棄すると全パネルを
  閉じ、runtime はホストを失ったときと同じく `disconnect` される。

## 実装

| ファイル | 責務 |
|---|---|
| include/tela/desktop_placement.hpp | 隅・置き場所・モニタの値と配置の計算 API（Core、Windows 非依存） |
| src/domain/desktop/desktop_placement.cpp | 四隅と余白、DPI 換算、所属モニタの決定、画面外からの復帰、移動後の置き場所 |
| include/tela/desktop_points.hpp / src/domain/desktop/desktop_points.cpp | ポイント座標の画面（macOS）での配置: 作業領域の換算、面を抱えた画面の倍率、AppKit 原点と Viewport の画素（Core） |
| include/tela/desktop_overlay_options.hpp | OS 共通の `DesktopOverlayOptions`（置き場所・つまみ・移動通知・取り込み除外） |
| include/tela/macos_desktop_overlay.hpp | macOS の面の API |
| src/infrastructure/macos/macos_desktop_overlay.mm | パネルの寿命、つまみのドラッグ、ポインタが乗っている領域、提示 |
| src/infrastructure/macos/desktop_screens.* | NSScreen の作業領域と倍率の読み取り |
| src/infrastructure/macos/desktop_panel.* | パネルを最前面・全スペース・切替対象外・取り込み除外にする |
| src/infrastructure/macos/region_panels.* | 排他領域の入力用パネル（MacOSOverlay と共用、host-overlay） |
| examples/macos/desktop_overlay_probe.mm | macOS の目視用の例 |
| tests/desktop_points_test.cpp | ポイント座標での配置の契約テスト |
| include/tela/windows_desktop_overlay.hpp | デスクトップに浮かぶ面の API |
| src/infrastructure/windows/windows_desktop_overlay.cpp | ウインドウの寿命、つまみのドラッグ、ポインタが乗っている領域、提示 |
| src/infrastructure/windows/desktop_monitors.* | モニタの作業領域と DPI の読み取り、Per-Monitor V2 への一時切替 |
| src/infrastructure/windows/layered_composition.* | 半透明の見た目用ウインドウと排他領域の入力用ウインドウ（WindowsOverlay と共用） |
| src/infrastructure/windows/native_pointer.* | 主ボタンと移動を runtime の入力へ変える（WindowsOverlay / WindowsView と共用） |
| examples/windows/desktop_overlay_probe.cpp, examples/desktop/desktop_probe_* | 固定の宣言を半透明で浮かべる目視用の例（宣言と引数は OS 共通） |
| tests/desktop_placement_test.cpp | 配置の契約テスト |
| tests/consumer | install した package から面をリンクする確認 |

## 検証

- `tela_desktop_placement_contract`（CTest）: 四隅と余白、DPI 換算、主モニタの判定、負の原点、
  端からはみ出した面の寄せ、2 つのモニタをまたぐ面の所属、画面外からの復帰、作業領域より大きい面、
  使えないモニタの除外と不正な値の拒否、移動後の置き場所とその往復。
- `tela_desktop_points_contract`（CTest）: macOS の作業領域（メニューバーと Dock）のポイント換算、
  下にずれた 2 枚目の画面、四隅と余白、倍率 2x/1x の Viewport、AppKit 原点、画面をまたぐ面の所属と倍率、
  画面外からの復帰、作業領域より大きい面、倍率の変化、端数の丸め、使えない画面の除外と不正な値の拒否、
  移動後の置き場所とその往復、外した画面からの復帰。
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
`exclude_from_capture` を立てたときに取り込みから外れること（Windows の画面取り込み・録画、
Windows 10 2004 未満での失敗）は未確認。

macOS 版は Windows 環境で実装し、AppKit のコンパイルも実行もしていない。Mac 拠点で
`-DTELA_BUILD_MACOS=ON` のビルド、`tela_macos_desktop_overlay_probe` での見え方・素通し・ボタン・
つまみ・Retina と複数画面・倍率変更・Spaces とフルスクリーン・ミッションコントロール・
`--exclude-from-capture` でのスクリーンショットと画面収録（ScreenCaptureKit を使う配信アプリを含む）
の確認が要る。
