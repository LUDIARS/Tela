---
task: spec-view-card-title-truncated
project: Tela
kind: 実装
created: 2026-09-16
memory_links:
  - spec/feature/spec-view.md
  - spec/contracts.md
  - spec/architecture.md
---
# 仕様カードの題名が 1 行で切れて読めない

## 目的

2026-09-16 の単体表示確認で、カードのコード行 (`TL-MANUAL-03 draft v2`) は読めるのに、
題名行が「説明書」だけで切れていた。元の題名は「説明書 3：作業画面に追従する表示」で、
カード幅に収まらない部分が次の行へ折り返されずに落ちている。
`TL-MANUAL-SOURCES` では「説明書の参照版と確認範」と語の途中で切れる。

仕様の区別はコードでできるので表示は成立しているが、題名が読めないと
可視化ビューとしての価値が下がる。Pf の画面側は題名を 3 行まで表示している
(`.spec-view-card-title` の line-clamp) ので、同じ絵にするには Tela 側も折り返しが要る。

## 完了条件

- Tela の text 宣言が幅に対して折り返すのか切り捨てるのかを確認し、現状の挙動を
  contracts に明文化する。
- カードの題名が card の高さに収まる範囲で複数行に折り返るようにする。
  収まらない分の扱い (省略記号 / 行数上限) を決めて contracts へ書く。
- 折り返しを Tela::Core の text 全体の挙動として変えるのか、仕様カード側で
  行を分けて宣言するのかを決める。前者なら既存の transition editor と probe の
  表示に退行が無いことを確認する。
- 実画面のキャプチャで題名が読めることを確認する。描画回数や要素数だけで合格にしない。
- Pf の画面と並べて、題名の見え方が揃っていることを確認する。

## スコープ (編集可ディレクトリ)

src/domain/composition、src/application/spec_view_document.cpp、spec/。
worktree からのサービス起動は禁止。
