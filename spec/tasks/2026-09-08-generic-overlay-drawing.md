---
task: generic-overlay-drawing
project: Tela
kind: 実装
created: 2026-09-08
memory_links:
  - spec/feature/overlay-drawing.md
  - spec/contracts.md
---
# 汎用描画と対象に線でつながる名前 UI

## 目的
Mp の名前 UI のようなオーバーレイを、対象アプリに依存しない部品として構成できるようにする。

## 完了条件
- 線・折れ線・実線／破線・矩形・角丸・楕円を宣言して描画できる。
- 対象に追従する引き出し線付きラベルが画面端で折り返し、対象喪失時に消える。
- DPI、クリップ、半透明、入力透過と変更時のみの更新を検証する。
- 利用例と既存 Pictor adapter への接続を用意する。

## スコープ (編集可ディレクトリ)
Tela の宣言 API、composition domain、CPU 描画 adapter、サンプル、仕様、契約テスト。
