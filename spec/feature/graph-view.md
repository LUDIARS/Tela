# Graph view

SPEC-TL-GRAPH。Pf のドメイン関係図を Tela で描く。価値は UX-TL-W5（[product](../ux/product.md)）。
Pf 側の対応は Praeforma の PF-DR-6（`spec/feature/domain-relations.md`）。

## 使い方

1. Pf のドメインタブの関係図で分類ごとの表示を決め、「この関係図を Tela 用に書き出す」でファイルを保存する。
   その時点の表示・非表示が初期表示になる。
2. 自前のウインドウで見る: `tela_view --font <file.ttf> --graph <file> [--fullscreen]`
3. 作業対象へ重ねる: `tela_overlay --font <file.ttf> --graph <file> --attach probe-target [--place right]`
4. 左上のボタン（ON/OFF と分類名）で分類ごとに表示を切り替える。切り替えは起動中だけで、
   ファイルや Pf へは書き戻さない。

`--graph` は他の内容（`--spec-view` / `--scene-overlay` / `--transitions` / `--probe`）と同時に指定できない。

## 契約

ファイル形式・上限・拒否条件・宣言の規則は [contracts](../contracts.md) の SPEC-TL-GRAPH。

- **Tela はレイアウトを計算しない。** ノードの位置も辺の経路も Pf が決めたものを描くだけで、
  曲線を引き直さない。画面と Tela が同じ形になることをこの分け方で担保する。
- 図は論理ピクセルのビューポートへ縦横比を保って中央に収める。
- 表示中の分類はノードを分類色の枠で描き、名前を枠内に収まる行数で重ねる。塗りは
  Pf のキャンバスと同じ混色にして、明るいホストでも文字が読めるようにする。
- 辺は両端のノードが表示されているときだけ描く。片方が隠れている関係は出さない
  （行き先の無い線を残さない）。所属は実線、親子は破線。
- 関係は有向なので、終端に矢じりを描く。向きは経路の最後の区間から取り、破線の関係でも
  向きが読めるよう矢じりは実線にする。配置が列でも円でも同じ規則で描く。
- 図形は1キャンバス200件ずつに分けて Drawing の上限（256件）内に収める。
- 読み込めないファイルは部分的に使わず、起動を失敗させる。

## 実装

| ファイル | 責務 |
|---|---|
| include/tela/graph.hpp | 値型・モデル・読み込み・宣言の API |
| src/application/graph.cpp | モデルの検証と表示状態 |
| src/application/graph_file.cpp | `TELA_GRAPH 1` の読み込み |
| src/application/graph_document.cpp | ビューポートへの収め方と宣言 |
| examples/windows/graph_content.* | ホストでの再宣言判定と切り替え |
| tests/graph_test.cpp | 読み込み・拒否・宣言・切り替え・上限の契約テスト（Pf の書き出しテストと同じバイト列） |

## 未確認

実画面での表示・分類の切り替え・破線の見え方は未確認。
ノード数が上限（128）に近いときの見やすさも未確認。
