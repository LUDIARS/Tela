# Scene overlay

SPEC-TL-SCENE-OVERLAY。Pf（Praeforma）の統合シーンエディタで重ねたシーンの仮配置を、
Unity の Scene ビューに重ねて描き、シーン単位で表示・非表示を切り替える。
Pf 側の対応は Praeforma の PF-SCENE-8（`spec/feature/integrated-scene-editor.md`）。

## 使い方

1. Pf のシーンエディタ「シーンの重ね合わせ」で画面を選び、「この画面を Tela 用に書き出す」でファイルを保存する。
   その時点の表示・非表示が初期表示になる。
2. `tela_overlay --font <file.ttf> --scene-overlay <file>` を Excubitor 経由で起動し、Unity の Tools/Tela から接続する。
3. 左上のボタン（ON/OFF とシーン名）でシーンごとに表示を切り替える。切り替えは起動中だけで、ファイルや Pf へは書き戻さない。

`--scene-overlay` は `--probe` / `--transitions` と同時に指定できない。

## 契約

ファイル形式・上限・拒否条件・宣言の規則は [contracts](../contracts.md) の SPEC-TL-SCENE-OVERLAY。

- 画面は論理ピクセルのビューポートへ縦横比を保って中央に収める。Pf 側で重ねたシーンの要素は書き出し時に基準画面の座標へ変換済みで、Tela は基準画面を一度だけ収める。
- 表示中のシーンは要素を色付きの枠とラベルで描き、入力はホストへ透過する。シーンの色は重なり順で決まる。
- 図形は1キャンバス200件ずつに分けて Drawing の上限（256件）内に収める。
- 読み込めないファイルは部分的に使わず、起動を失敗させる。

## 実装

| ファイル | 責務 |
|---|---|
| include/tela/scene_overlay.hpp | 値型・モデル・読み込み・宣言の API |
| src/application/scene_overlay.cpp | モデルの検証と表示状態 |
| src/application/scene_overlay_file.cpp | `TELA_SCENE_OVERLAY 1` の読み込み |
| src/application/scene_overlay_document.cpp | ビューポートへの収め方と宣言 |
| examples/windows/scene_overlay_content.* | ホストでの再宣言判定と切り替え |
| tests/scene_overlay_test.cpp | 読み込み・拒否・宣言・切り替え・上限の契約テスト（Pf の書き出しテストと同じバイト列） |

## 未確認

実 Unity での表示位置・切り替え操作・負荷は、Tela の Unity 実機受入（[windows-composition](../windows-composition.md)）と同じく未確認。
