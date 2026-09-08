# Overlay drawing

SPEC-TL-DRAWING。汎用 UI と対象に線でつながるオーバーレイを Tela の宣言 API で構成する。
Mp (`MakaiNuiPictor/web/js/callout.js`) の名前 UI を参照し、対象データへの依存を持ち込まない。

## 契約

- Drawing は矩形、角丸矩形、楕円、線・折れ線を値として宣言する。塗り、線色、太さ、
  線の実線／破線を指定する。破線の枠は明示的な折れ線として指定する。
- canvas は既存の親レイアウト・クリップ・宣言順に従う。座標と線幅は論理ピクセル。
  変換は描画 adapter で一度だけ行い、入力の既定は透過。
- Drawing が同じなら Runtime の再宣言で再描画を要求しない。形状やスタイルの変更は更新する。
- callout は親の論理座標内の anchor と表示可能領域を受け取る。右上などの相対配置を指定し、
  画面端では折り返して余白内へ収め、線をラベルの境界まで伸ばす。
- 対象が非表示・画面外、または領域がラベルより小さい場合はラベルと線を宣言しない。
  ホストは対象の移動・可視性変化時に同じ ID で再宣言する。
- 座標がデスクトップ物理ピクセルなら `(target - viewport origin) / dpi_scale` で変換して渡す。
  Unity/Mp のワールド座標の投影はホスト側が担当する。
- 複数ラベル間の重なり回避、曲線、任意 SVG、画像・WebView の描画はこの primitive API の対象外。
  CEF WebView は専用 adapter の契約で扱う。

## 検証

有限値・描画予算の検証、DPI、クリップ、半透明画素、破線、画面端の折り返し、
対象非表示時の消去、宣言差分と入力透過を契約テストで確認する。
Windows/Mac の実表示は各プラットフォームの本体で別途受入検証する。

## 利用例

`examples/overlay_drawing.cpp` に図形と名前ラベルの宣言例を置く。
既存の Windows probe にも追従ラベルを追加し、viewport revision が変わったときに
再宣言する。ボタンはクリック数だけを更新し、ProbeContent が viewport revision と
クリック数を比較して宣言を更新する。通常の待機ループで毎回同じ Document を作り直さない。

```cpp
tela::Callout name;
name.anchor = {target_x, target_y}; // parent-local logical pixels
name.visible = target_visible;
name.leader = {{240,240,250,255}, 1.5f, 6, 4};
tela::callout(document, "object.name", "KENZOKU", name, {0,0,width,height});
```

Drawing は最大 256 コマンド・合計 4096 点。CPU Raster は呼び出し単位で
画素×セグメント評価の予算を検査し、過大な描画を出力変更前に拒否する。
半透明は premultiplied BGRA の source-over 合成。アンチエイリアスは画素中心から
境界までの距離による被覆率で、楕円の境界距離は近似を使用する。
