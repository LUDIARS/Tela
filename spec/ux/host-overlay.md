---
title: 作業対象に追従するツール表示
type: feature
ux_definition: 1
id: UX-TL-OVERLAY
service: tela
domain: host-overlay
ux_scope: core-domain
product_ux: spec/ux/product.md
status: draft
owner: product-owner
---
# 作業対象に追従するツール表示
## 誰の、どの状況の問題か
制作者が別ウインドウの設定とScene内の対象を往復し、対応する対象を見失う。
## このプロダクトが何を解決するか
プロダクトUXのW2/W4へ、対象と表示面の対応・可視性・寿命の規則で貢献する。
HWND生成は支援実装で、コアの意味は対象を追い続けること。
## どの価値を実現するか
| 価値 ID | 望ましい変化 | 判断方法・条件 | 証拠 | 現状 |
|---|---|---|---|---|
| UX-TL-O1 | 移動しても対象とツール表示が対応する | 移動・DPI・隠蔽・復帰シナリオ | windows-composition acceptance | 未測定 |
## 主要シナリオと失敗からの回復
対象を選択→位置に表示→対象移動に追従。対象喪失時は隠して入力を解除し、新しい接続で復帰する。
## 守る制約と優先順位
画面座標は負の原点を許す物理ピクセル。隠れたホストの上へ表示し続けない。内容変更がなければ描画提出しない。
## 対応関係・非目標・未決事項
O1→SPEC-TL-OVERLAY。Vulkan直接合成は別検証。Windows CPU合成の実機合格は未判定。
