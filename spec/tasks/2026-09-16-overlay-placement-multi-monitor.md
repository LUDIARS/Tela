---
task: overlay-placement-multi-monitor
project: Tela
kind: テスト
created: 2026-09-16
memory_links:
  - spec/feature/overlay-placement.md
  - spec/contracts.md
  - spec/windows-composition.md
---
# ウインドウ外配置を複数モニタと DPI 差で確認する

## 目的

`--place` の置き場所は対象ウインドウのモニタの作業領域だけを見て決めている。
2026-09-16 の確認は 1 台のモニタ上でしか行っておらず、モニタをまたぐ配置と、
DPI の違うモニタ間での見え方は未確認のまま spec に残している。

配置は対象のモニタの `MonitorFromWindow(MONITOR_DEFAULTTONEAREST)` で決め、
大きさは対象ウインドウの DPI で拡大している。対象が DPI の違うモニタへ移ったときや、
隣のモニタへはみ出す位置に置かれたときの挙動を確かめる。

## 完了条件

- DPI の異なる 2 台のモニタで、対象ウインドウを各モニタに置いて表示位置と文字の大きさを確認する。
- 対象をモニタ間で移動したとき、配置と拡大率が追従することを確認する。
- 隣のモニタへまたぐ位置になったとき、反対側へ回るか作業領域内へ寄るかが
  contracts の記述どおりであることを確認する。
- タスクバーの位置を変えたときに作業領域の判定が追従することを確認する。
- 結果を spec/feature/overlay-placement.md の「未確認」から実績へ移す。

## スコープ (編集可ディレクトリ)

spec/feature/overlay-placement.md の受入記録、必要なら
examples/windows/attached_viewport.cpp。worktree からのサービス起動は禁止。
