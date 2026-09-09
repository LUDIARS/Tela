# macOS composition [id: SPEC-TL-MACOS]

Tela の Mac 表示 adapter。UX は「LUDIARSの柔軟なツール描画」。
AppKit ウインドウの寿命と入力は host-overlay、文字図形の描画は既存の
pictor-composition に属する。Core に Objective-C / AppKit を持ち込まない。

## ビルドと利用

macOS の Apple Clang / CMake 3.20 以降、同一 CPU 向けにビルドした Pictor と
TrueType outlines のフォントを明示的に用意する。arm64 / x86_64 はビルド先の
アーキテクチャを使用する。Windows の pictor.lib は使用できない。

```sh
cmake -S . -B build-native -DTELA_BUILD_MACOS=ON \
  -DTELA_PICTOR_INCLUDE_DIR=/absolute/pictor/include \
  -DTELA_PICTOR_LIBRARY=/absolute/pictor/lib/libpictor.a
cmake --build build-native --config Release
```

インストール後は `find_package(Tela CONFIG REQUIRED)` と `Tela::MacOS` を使う。
描画面だけ必要な利用者は `TELA_BUILD_PICTOR=ON` / `Tela::Pictor` を選択できる。
Pictor が共有ライブラリなら、その実行時配置は利用アプリが担当する。

`MacOSOverlay(runtime, renderer, CGWindowID)` は別アプリを含む対象ウインドウに
追従する。NSApplication とイベントループは呼び出し元が所有する。
生成・同期・非表示・破棄はすべて AppKit メインスレッドで行い、Runtime と
PictorSurface は overlay より長生きさせる。対象はウインドウ全体であり、
Unity Scene の部分領域や任意 NSView への接続はこの adapter では提供しない。

`examples/macos/overlay_host.mm` は対象 ID、フォント、継続秒数（既定120、最大3600）を
引数に取る有限時間のサンプル。対象情報を可視時50ms・非表示時250msで観測する。
ライブラリ自体はタイマー、スレッド、イベントタップ、IPC を開始しない。
起動検証は Mac の本体フォルダから Excubitor に登録した実行構成を使用し、
Concordia testing claim / release を行う。worktree から起動しない。

## 表示・座標

- 透過する視覚面は nonactivating NSPanel に Pictor の premultiplied BGRA8 を合成する。
  CoreGraphics へのコピーが画素の所有権を保持し、描画後の vector に依存しない。
- CGWindowID の owner、bounds、onscreen と前面アプリを観測する。対象喪失、
  最小化・非表示、別アプリが前面、画面外では隠して操作をキャンセルする。
  一度喪失を観測した ID は再利用しない。権限等で情報取得不能なら明示エラーにする。
- AppKit の左下原点を、主画面上端を基準に左上原点へ変換する。
  最も広く重なる画面の backingScaleFactor を使用し、Retina ではその倍率で面を作る。
- macOS の複数画面には一意な全体の物理ピクセル格子がない。この adapter の
  Viewport は対象画面の倍率でグローバル point 座標全体を変換した座標基準を使う。
  画面移動で倍率が変われば origin も revision も変わる。ホストからの anchor や
  shared 観測も同じ revision と倍率で変換する。別画面の倍率を混ぜない。
- ウインドウ移動・サイズ・倍率・可視性で再配置する。同じ宣言の待機中は
  bitmap の再生成と提出を行わない。複数倍率の画面をまたぐ面は対象画面の倍率で
  一枚描画し、別画面部分は OS が拡縮する（画面ごとの分割ラスタライズはしない）。

## 入力・寿命

視覚面は全体をクリック透過にする。実効 exclusive fragments だけ別の
nonactivating panel で受け、shared と passthrough 部分は元アプリへ届く。
操作領域の最終画素をその panel へ移し、視覚面では同じ画素を消して二重合成を避ける。
macOS は完全透明な画素をヒット対象にしないため、明示 exclusive 領域内だけは
alpha を最低 1/255 にする。これは透明色を指定した操作領域の表示上の制約でもある。
AppKit が down を受けた view に drag/up を配送するため、ドラッグ中はその panel を
保持する。移動・倍率変更・対象喪失ではキャンセルし、古い panel を閉じる。
ボタン動作は Runtime が一度だけ実行する。物理イベントを OS に再注入しない。
一次ボタンに対応し、文字入力・IME・キーボードフォーカス・右クリックメニューは
この adapter の提供範囲外。フォルダメニューは別契約で扱う。

`hide()` は現在の表示と native 操作領域を解除する。再度 synchronize すれば
対象状態に応じて復帰する。接続を終了するホストは同期を止め、adapter を破棄する。
入力コールバックの C++ 例外は保存し、次の同期で通知して全 panel を隠す。

## 検証の境界

この変更は Windows 環境で実装した。AppKit のコンパイル、表示の上下・色・透明度、
透明入力 panel の命中、ドラッグ、Retina と複数画面、Spaces/全画面、画面収録権限の
有無、待機 CPU/GPU と終了時の資源解放は Mac 実機での確認が必要。
Windows のビルド成功やソース確認は Mac 受入成功ではない。
Mac の CEF host は Orbis の別タスクであり、この表示 adapter で実装済みとはしない。

一次資料:
- https://developer.apple.com/documentation/appkit/nswindow
- https://developer.apple.com/documentation/appkit/nswindow/windownumber(at:belowwindowwithwindownumber:)
- https://developer.apple.com/documentation/coregraphics/cgwindowlistcopywindowinfo(_:_:)
- https://developer.apple.com/library/archive/documentation/Cocoa/Conceptual/EventOverview/HandlingMouseEvents/HandlingMouseEvents.html
