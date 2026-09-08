---
title: ホストをまたぐツール構成
type: feature
ux_definition: 1
id: UX-TL-COMPOSITION
service: tela
domain: tool-composition
ux_scope: core-domain
product_ux: spec/ux/product.md
status: draft
owner: product-owner
---
# ホストをまたぐツール構成
## 誰の、どの状況の問題か
作者が同じ操作を別ホストへ移すたび、UI状態や配置まで書き直している。
## このプロダクトが何を解決するか
プロダクトUXのW1へ、宣言・安定ID・操作の一貫した構成モデルで貢献する。
ボタンという部品自体ではなく、作者が組み替えても意味と状態を保つ規則をコアとする。
## どの価値を実現するか
| 価値 ID | 望ましい変化 | 判断方法・条件 | 証拠 | 現状 |
|---|---|---|---|---|
| UX-TL-C1 | 文言や構成を変えても対象の状態が意図せず失われない | 同じIDの再宣言、削除、操作途中の変更を確認 | runtime contract | 契約確認済み |
## 主要シナリオと失敗からの回復
宣言→レイアウト→表示→操作→データ更新。重複IDや不正寸法は宣言時エラーとし、失敗したパネルを原子的に巻き戻す。
## 守る制約と優先順位
UIの意味をOSやUnityの型で表さない。IDは文言から生成しない。
## 対応関係・非目標・未決事項
C1→SPEC-TL-DECLARATION／SPEC-TL-RUNTIME。通常ウインドウのホスト適合は今後の利用先。
各ウィジェットを個別bounded contextにする必要性は現時点でない。分類は設計案。
