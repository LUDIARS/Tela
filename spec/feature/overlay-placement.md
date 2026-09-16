# Overlay placement

SPEC-TL-PLACEMENT。読み取り専用の表示を、対象ウインドウの内側ではなく隣に置けるようにする。

## 目的

仕様書可視化ビューのような「見ながら作業する資料」は、作業対象の上に重ねると
対象を隠してしまう。また対象より大きい図は内側に収めると縮小され、文字が読めなくなる。
対象の隣に等倍で置けば、両方を同時に見られる。

## 使い方

`--attach probe-target` と併せて `--place <inside|left|right|above|below>` を指定する。
既定は `inside` で従来どおり対象の内側に収める。

```sh
tela_overlay --font <file.ttf> --spec-view <file> --attach probe-target --place right
```

文字の大きさは `--font-size <8..96>` で選ぶ。ホストの大きさは実行ごとに違うのに、
読み込むファイルは同じなので、大きさは起動時の選択にする。書体は従来どおり `--font` で渡す。

## 契約

規則は [contracts](../contracts.md) の SPEC-TL-PLACEMENT。

- `inside` は対象のクライアント領域をそのままビューポートにする。それ以外は
  **内容が自分で持っている大きさ**（書き出しに入っている図の寸法）でビューポートを作り、
  対象ウインドウの外側へ置く。縮小されないので文字が読める。
- 置き場所がモニタの作業領域からはみ出すときは反対側へ回す。両側とも入らないときは
  作業領域内へ寄せる。見えないより重なる方がましとする。
- 作業領域より大きい図は作業領域の大きさまで縮める。
- `inside` 以外は `--attach probe-target` が要る。Unity 経由ではビューポートを Unity が決めるため、
  セッション側が勝手に置き場所を変えない。
- 対象が隠れた・最小化された・消えたときの扱いは `inside` と同じ。隣に置いても
  対象の寿命に従う。

## 実装

| ファイル | 責務 |
|---|---|
| include/tela/placement.hpp | 置き場所の値と計算 API |
| src/domain/overlay/placement.cpp | 作業領域を踏まえた矩形の決定 |
| examples/windows/attached_viewport.hpp/.cpp | 対象ウインドウとモニタからビューポートを作る |
| examples/windows/overlay_options.* | `--place` と `--font-size` の受け取りと排他 |
| tests/placement_test.cpp | 置き場所・反対側への回り込み・作業領域の上限の契約テスト |

## 未確認

複数モニタ、DPI が異なるモニタ間での見え方は未確認。
Unity の Scene ビューの隣へ置くことは対象外（Unity 側の対応が要る）。
