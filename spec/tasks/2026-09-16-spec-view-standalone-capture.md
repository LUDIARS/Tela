---
task: spec-view-standalone-capture
project: Tela
kind: テスト
created: 2026-09-16
memory_links:
  - spec/feature/spec-view.md
  - spec/contracts.md
  - spec/windows-composition.md
  - spec/implementation-status.md
---
# 仕様書可視化ビューの単体表示を本体で確認してキャプチャを残す

## 目的

`--spec-view` と `--attach probe-target` で、Pf が書き出した仕様書可視化ビューが
実画面に描かれることを確認する。Unity 実機受入が未確認のままでも表示を検証・撮影できる
経路として成立しているかを見る。撮る対象は Tela 自身の仕様 (dogfooding)。

## 完了条件

- 本体を main 最新へ同期し、MSVC Release でビルドする。worktree のビルド成果物を使わない。
- Pf から書き出した `TELA_SPEC_VIEW 1` を `build-native/` に置き、起動前に Concordia へ testing claim を入れる。
- Excubitor 経由で `tela-probe-target` と `tela-spec-view` を起動する。直接起動しない。
- Tela の仕様カードが分類軸の 2 グループで描かれた実画面キャプチャを保存する。
  カードのコード行・題名 (日本語) が読めることを目視で確認し、描画回数だけで合格にしない。
- グループの表示を切り替えた後のキャプチャも保存し、切り替えが効くことを確認する。
- Pf 側の同じ図と見比べ、配色と配置が一致することを確認する。
- 終了後にプロセスの終了を確認し、claim を release する。

## スコープ (編集可ディレクトリ)

Tela 本体の build-native 成果物とキャプチャ。worktree 起動は禁止。
Unity 連携と September 連携は対象外。
