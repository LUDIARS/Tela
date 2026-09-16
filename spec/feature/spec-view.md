# Spec view

SPEC-TL-SPEC-VIEW。Pf（Praeforma）の仕様書可視化ビューで作った図を、作業画面に重ねて描き、
グループ単位で表示・非表示を切り替える。Pf 側の対応は Praeforma の PF-SPEC-VIEW
（`spec/feature/spec-visualization-view.md`）。

## 使い方

1. Pf の仕様タブ「可視化」で軸（状態・分類・優先度）と各グループの表示を決め、「この図を Tela 用に書き出す」でファイルを保存する。その時点の表示・非表示が初期表示になる。
2. `tela_overlay --font <file.ttf> --spec-view <file>` を Excubitor 経由で起動する。Unity の Scene ビューに重ねる場合は Tools/Tela から接続する。
3. Unity を使わずに単体で見る場合は `tela-probe-target` を先に起動し、`--attach probe-target` を足して起動する。
   カードを対象ウインドウの隣に等倍で置くときは `--place right` などを足す
   （[overlay placement](overlay-placement.md)）。文字の大きさは `--font-size` で選ぶ。
4. 左上のボタン（ON/OFF とグループ名）でグループごとに表示を切り替える。切り替えは起動中だけで、ファイルや Pf へは書き戻さない。

`--spec-view` は `--probe` / `--transitions` / `--scene-overlay` と同時に指定できない。
`--attach probe-target` は `--spec-view` か `--scene-overlay` が要る（`--probe` は既に同じ窓を持つ）。

## 契約

ファイル形式・上限・拒否条件・宣言の規則は [contracts](../contracts.md) の SPEC-TL-SPEC-VIEW。

- 図は論理ピクセルのビューポートへ縦横比を保って中央に収める。カードの座標は Pf の書き出し時に
  確定しており、Tela は図全体を一度だけ収める。
- 表示中のグループはカードを色付きの枠で描き、コード行（`コード 状態 v版`）と題名行を重ねる。
  入力はホストへ透過する。グループの色は並び順で決まり、Pf の `SPEC_VIEW_PALETTE` と同じ並びを使う。
  カードの塗りは Pf の暗いキャンバスと同じ混色を先に済ませた色にする。オーバーレイは自前の
  キャンバスを持たないので、そうしないと明るいホストの上で文字が読めなくなる。
- 図形は1キャンバス200件ずつに分けて Drawing の上限（256件）内に収める。
- 読み込めないファイルは部分的に使わず、起動を失敗させる。

## 実装

| ファイル | 責務 |
|---|---|
| include/tela/spec_view.hpp | 値型・モデル・読み込み・宣言の API |
| src/application/spec_view.cpp | モデルの検証と表示状態 |
| src/application/spec_view_file.cpp | `TELA_SPEC_VIEW 1` の読み込み |
| src/application/spec_view_document.cpp | ビューポートへの収め方と宣言 |
| examples/windows/spec_view_content.* | ホストでの再宣言判定と切り替え |
| examples/windows/overlay_options.* | `--spec-view` と `--attach` の受け取りと排他 |
| tests/spec_view_test.cpp | 読み込み・拒否・宣言・切り替え・上限の契約テスト（Pf の書き出しテストと同じバイト列） |

## 未確認

実 Unity での表示位置・切り替え操作・負荷は、Tela の Unity 実機受入（[windows-composition](../windows-composition.md)）と同じく未確認。
`--attach probe-target` での単体表示は、この受入とは別に実行して確認する。
