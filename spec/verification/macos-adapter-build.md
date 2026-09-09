# Mac adapter の実装時確認（2026-09-09）

対象: `spec/macos-composition.md`。基点は Tela `921b63677e19`。

- Windows x64 / MSVC 19.39、Windows SDK 10.0.19041 で既存の
  `TELA_BUILD_WINDOWS=ON` 構成を Release ビルドし、成功した。
- `TELA_BUILD_PICTOR=ON`、Windows adapter 無効の別構成でも
  `tela_pictor` の Release ビルドが成功した。
- install export 後、別の CMake consumer で `find_package(Tela)` と
  `Tela::Pictor` を使う実行ファイルのリンクが成功した。実行はしていない。
- 差分の空白検査を実施。起動・単体・統合テストは実行していない。

Mac SDK / AppKit / Mac 実機はこの環境にない。Objective-C++ ソースのネイティブ
コンパイル、透明合成・入力・倍率・Spaces・性能の成功をこの記録から主張しない。
必要な受入条件は `spec/tasks/2026-09-09-macos-overlay-native-acceptance.md`。
