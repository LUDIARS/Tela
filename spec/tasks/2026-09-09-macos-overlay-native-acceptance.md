---
task: macos-overlay-native-acceptance
project: Tela
kind: 検証
created: 2026-09-09
memory_links:
  - spec/macos-composition.md
  - spec/contracts.md
---
# AppKit overlay の Mac 実機受入

## 目的
Mac 用 adapter とサンプルをネイティブビルドし、透明表示・入力・寿命の契約を確認する。

## 完了条件
- 同一アーキテクチャの Pictor を用いて Tela::MacOS とサンプルをビルドする。
- 本体フォルダの Excubitor 構成を使用し、Concordia claim/release 付きで起動する。
- 文字図形の向き・色・透明度、透過領域とボタン、領域外までのドラッグを確認する。
- Retina、倍率の異なる複数画面、移動・縮小・復帰・対象終了で追従と解除を確認する。
- 非表示時の描画停止、待機負荷、終了後の panel/bitmap 解放を記録する。
- 権限がない場合の挙動と Spaces / 全画面の対応条件を記録する。

## 範囲
Tela の Mac 本体と受入記録。September 統合と Orbis の CEF Mac 移植は含めない。
