---
task: spec-view-controls-cover-first-card
project: Tela
kind: 実装
created: 2026-09-16
memory_links:
  - spec/feature/spec-view.md
  - spec/feature/overlay-placement.md
  - spec/contracts.md
---
# コントロールパネルが先頭のカードを覆う

## 目的

2026-09-16 の実機表示で、左上のグループ切り替えパネルが図の左上に重なり、
最初のカード (TL-MANUAL-01) が完全に隠れていた。内側配置でも外側配置でも同じで、
カードが増えても先頭は常に読めない。

パネルは図と同じ宣言の中に絶対座標で置いているため、図の 1 枚目と必ず重なる。
シーン書き出し (scene overlay) では要素が枠とラベルだけなので目立たなかったが、
仕様カードは中身を読むものなので隠れると困る。

## 完了条件

- パネルとカードが重ならないようにする。図の余白を空けてパネルを置く、パネルの分だけ
  図を下げる、パネルを別の辺へ寄せる、のどれを採るかを決めて contracts に書く。
- 決めた方法が Pf の画面と同じ絵になる条件を壊さないことを確認する。Pf 側の配置を
  変える必要があるなら、Pf の PF-SPEC-VIEW と対で直す。
- グループ数が 1 のときと上限 (32) のときで、パネルが図をどれだけ押し出すかを確認する。
- 契約テストに、パネルの矩形とカードの矩形が重ならないことを入れる。
- 実画面のキャプチャで先頭カードが読めることを確認する。

## スコープ (編集可ディレクトリ)

src/application/spec_view_document.cpp、spec/。必要なら Praeforma の shared/spec-view.ts と対で。
worktree からのサービス起動は禁止。
