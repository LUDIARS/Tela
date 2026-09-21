---
task: desktop-overlay
project: Tela
kind: 実装
created: 2026-09-21
memory_links:
  - spec/feature/desktop-overlay.md
  - spec/contracts.md
  - spec/windows-composition.md
  - spec/ux/product.md
---
# デスクトップに単独で浮かぶ半透明の面 WindowsDesktopOverlay を足す

## 目的

作業中ずっと見ておきたい情報（Lares の部屋の状態や今日の TODO など）を、ブラウザを開かずに
画面の隅へ浮かべたい。Tela にある面は、対象ウインドウに貼り付く `WindowsOverlay` と、不透明な
通常ウインドウ `WindowsView` だけで、対象を持たずにデスクトップへ浮かぶ半透明の面が無い。
これを Lares 専用ではなく Tela の汎用の面として足す（2026-09-21 neco 承認）。

下の作業を妨げないことを最優先にする。クリックとキー入力を奪わず、最前面にあっても下が
透けて見え、操作できること。半透明の出し方と入力の受け方は既存の合成を切り出して共用し、
2 つ目を作らない。Lares 側のアプリは別の委託で、このタスクの範囲は Tela 側だけ。

## 完了条件

- C-1 place_on_desktop(placement, monitors): 隅を指定した面は、主モニタの作業領域の隅から
  余白（論理ピクセル × そのモニタの DPI）の位置に、論理サイズ × DPI の大きさで置かれる
- C-2 place_on_desktop(placement, monitors): 絶対座標の面は、作業領域に乗る割合が最大のモニタに属して
  その作業領域の中へ寄せられる。どのモニタにも乗らなければ、主モニタの同じ隅へ戻って recovered になる
- C-3 place_on_desktop(placement, monitors): 作業領域より大きい面は作業領域まで縮め、不正な大きさや
  余白、使えるモニタが無い場合は std::invalid_argument で拒否する
- C-4 placement_at(current, x, y, monitors): 移動後の位置を作業領域の中へ寄せた絶対座標と、いちばん近い隅
  および論理余白で返す。その値を渡し戻すと同じ枠に置かれる
- C-5 WindowsDesktopOverlay(runtime, renderer, options): 見た目用ウインドウが LAYERED・TRANSPARENT・
  NOACTIVATE・TOOLWINDOW・TOPMOST で、排他領域だけが当たり判定を持ち、それ以外は下のウインドウへ
  素通しする。起動してもフォアグラウンドのウインドウが変わらない
- C-6 WindowsDesktopOverlay::synchronize(): つまみのドラッグで面ごと動き、離した位置を moved へ
  synchronize() の中で通知する（実際のマウス操作による目視は未確認で、手順は windows-composition.md にある）
- C-7 LayeredComposition::present(runtime, surface): WindowsOverlay と WindowsDesktopOverlay が同じ合成を使い、
  WindowsOverlay と WindowsView も同じ NativePointer を使う。既存の CTest がすべて通る
- C-8 find_package(Tela CONFIG): install した package から、別の CMake プロジェクトが Tela::Windows の
  WindowsDesktopOverlay をリンクできる（tests/consumer）
- C-9 ~WindowsDesktopOverlay(): ウインドウ・キャプチャ・入力領域を解放する。作成と破棄を繰り返しても
  GDI と USER のオブジェクト数が増えない

## スコープ (編集可ディレクトリ)

include/tela、src/domain/desktop、src/infrastructure/windows、examples/windows の desktop_* 、
tests（desktop_placement_test と consumer）、CMakeLists.txt、spec、README.md、excubitor.catalog.yaml。
Unity 側（unity/）と Lares リポは触らない。
