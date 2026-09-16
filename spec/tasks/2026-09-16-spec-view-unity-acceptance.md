---
task: spec-view-unity-acceptance
project: Tela
kind: テスト
created: 2026-09-16
memory_links:
  - spec/feature/spec-view.md
  - spec/windows-composition.md
  - spec/implementation-status.md
  - spec/tasks/2026-09-16-spec-view-standalone-capture.md
---
# 実 Unity の Scene ビューで仕様書可視化ビューの表示を確認する

## 目的

仕様書可視化ビューを `--spec-view` だけで起動し、Unity の Tools/Tela から接続した状態で
表示位置・グループ切り替え・入力の共存・負荷を確認する。単体の probe target 表示
([[spec-view-standalone-capture]]) では分からない、実ホストに追従する側の受入を埋める。

## 完了条件

- Excubitor 経由で `tela_overlay --font <ttf> --spec-view <file>` を本体から起動する。
- Unity の Scene ビューへ接続し、図がビューポートへ縦横比を保って収まることを確認する。
- 浮動 Scene ビューと DPI 混在のホストウインドウ同定を確認する。現状の実装は接続時に
  前面の Unity HWND を選ぶため、そこが崩れないかを見る。
- グループ切り替えの操作が Unity 側の入力と競合しないことを確認する。
- 再接続後に表示が復帰すること、idle の CPU とネイティブハンドルが増え続けないことを確認する。
- Scene overlay の Unity 受入 (windows-composition) と同じ手順・同じ証跡の形で記録する。

## スコープ (編集可ディレクトリ)

Tela 本体の build-native 診断成果物、spec/windows-composition.md の受入記録。
worktree 起動は禁止。Unity プロジェクトの既存変更には触らない。
