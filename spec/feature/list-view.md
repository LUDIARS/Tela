# List view

SPEC-TL-LIST-VIEW。他のプログラムが書き出した「印つきの行の一覧」(`TELA_LIST_VIEW 1`) を、デスクトップに浮かぶ
半透明の面 ([desktop overlay](desktop-overlay.md)) へ出し、ファイルが置き換わるたびに描き直す。
2026-10-31 Discord フォーラムでの依頼「Ex に通信監視を入れる…国外ドメインへの通信をマークし、Tela でオーバーレイする UI に
通信先とドメイン情報わかればそれを出す」の Tela 側。Actio: `actio:6e0645ec-296d-4070-b817-fa134f0b475f`
(Excubitor 側は `actio:928e7b31-c1f4-4604-9759-487745de243c`、Excubitor の `spec/feature/network-watch.md`)。

## 目的

作業中ずっと見ておきたい一覧を、ブラウザや専用のウインドウを開かずに画面の隅へ浮かべる。最初の利用先は
Excubitor の通信監視で、国外への通信の行を目立たせて出す。

Tela は一覧の意味を知らない。書く側が行ごとに印 (`alert` / `normal` / `muted`) を決め、Tela はその印どおりに
描くだけにする。通信監視の名前・国・ドメインといった語を Tela に持ち込まない (別の用途でもそのまま使える)。

## 使い方

```sh
tela_list_overlay --font <file.ttf> --file D:/LUDIARS/Excubitor/data/netwatch/overlay.telalist
```

- Excubitor 経由で起動する (`excubitor.catalog.yaml` の `tela-list-overlay`、既定は自動起動しない)。
- 右上の隅 (`--corner` で変更) に出る。つまみ (右上の三本線) をドラッグすると動く。
- 動かした後の位置は標準出力に `moved x y corner margin_x margin_y` と出るだけで、保存はしない。
- `--width` / `--rows` / `--font-size` で大きさ、`--exclude-from-capture` で画面の取り込みから外す。
- 止めるまで動き続ける (`--seconds` を付けたときだけ自分で終わる)。

## 形式と描き方

規則は [contracts](../contracts.md) の SPEC-TL-LIST-VIEW。

```
TELA_LIST_VIEW 1
title\tExcubitor 通信先
status\t国外 2 件
row\talert\tUS\tapi.example.com\t203.0.113.5:443\t10:42:03
row\tmuted\t??\t(ドメイン不明)\t192.0.2.9:443\t10:41:58
row\tnormal\tJP\tjp.example\t198.51.100.1:443\t10:41:40
```

- `alert` の行は赤い帯の上に `! ` を先頭に付けて出す。`muted` は `? ` を付ける。色が見分けにくくても印が読める。
- 列は 3 つの空白でつないで 1 行に出す。はみ出した分は面で切れる。
- 面に収まらない行は宣言せず、最後に `+N more` と数だけ出す。
- 読めない・壊れたファイルは、理由を状態の行に出して行を消す。前の行を残して「今の状態」に見せない。

## 実装

| ファイル | 責務 |
|---|---|
| include/tela/list_view.hpp | 値型・読み込み・宣言の API |
| src/application/list_view_file.cpp | `TELA_LIST_VIEW 1` の読み込みと検証 |
| src/application/list_view_document.cpp | 背景・見出し・状態・行・つまみの宣言 |
| include/tela/utf8.hpp, src/domain/text/utf8.cpp | 正規 UTF-8 の検証 (source view から切り出して共用) |
| examples/desktop/list_file_source.* | ファイルの時刻と大きさを 1 秒ごとに見て、変わったときだけ読み直す |
| examples/desktop/list_overlay_options.* | 引数 |
| examples/windows/list_overlay_host.cpp | デスクトップの面に出して描き直すループ |
| tests/list_view_test.cpp | 読み込み・拒否・上限・宣言・ファイル追従・引数の契約テスト (CTest `tela_list_view_contract`) |

## 検証

- 2026-10-31: `tela_list_view_tests`・`tela_list_overlay`・`tela_source_tests`・`tela_source_options_tests` を
  Release でビルドし、警告・エラーなしを確認した。**テストは実行していない** (実行の指示が無いため)。
- 実機での見え方 (半透明・赤い帯・文字の読みやすさ・つまみでの移動・ファイル更新への追従) は未確認。

## 未確認

- 実際の画面での見え方と、明るい背景の上での赤い帯と文字の読みやすさ。
- 長時間動かしたときの GDI / USER オブジェクト数 (面そのものは desktop overlay の確認に従う)。
- 書く側が止まった (ファイルが古いまま) ことは分からない。書く側は内容が変わったときだけ書くため、
  時刻での判定はしていない。必要なら書く側の状態の行に時刻を入れる。
- macOS のホストは無い (一覧の読み込みと宣言は OS 共通なので、`MacOSDesktopOverlay` で同じループを書けば出せる)。
