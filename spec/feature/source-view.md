# Dedicated source viewer

SPEC-TL-SOURCE-VIEW implements UX-TL-W6 (2026-09-16 neco: Tela 内で読む /
ソースコード専用の表示ツールが欲しい / 実装して).

The user must be able to read the implementation behind an Anatomia domain without
starting an editor or losing the referenced file and line. `tela_source` is a
separate native, read-only window using Tela's Windows/Pictor adapters.

## Invocation and acceptance

`tela_source --font <monospace.ttf> --source <UTF-8 file> [--line <1-based line>]`

`--font-size` accepts 8..48 (default 16). A monospace TrueType font with coverage
for the source language is supplied by the caller. Windows arguments use Unicode.
The window shows the path, current line range, horizontal character offset,
original line numbers, and highlights the requested line. Previous/next page,
left/right, first and target-line buttons preserve the loaded file.
Long lines are displayed in bounded slices; tabs expand at 4-character stops.
Empty files show line 1. CRLF and LF retain original line numbering; bare CR is
also treated as a line separator. UTF-8 BOM is accepted, malformed UTF-8, NUL and
unsupported control characters are explicit errors. Input is at most 8 MiB,
200,000 lines, and 65,536 bytes per line. No shell or source-code execution occurs.

The file adapter reads one bounded snapshot. SourceView owns immutable lines and
navigation state. The document function is a pure projection of that state and
the viewport. Window callbacks update navigation only. At most 200 rows are
declared, so a large source file never creates a document per file line.
Invalid requested lines fail visibly rather than opening unrelated code.
File errors leave disk contents untouched; fix the path/encoding and reopen.

Anatomia integration contract is an argument array containing `--source` and
`--line`, with no shell interpolation. The Anatomia domain launch action itself
is a separate adapter; this change supplies the receiving viewer executable.

## Validation status

Contract tests accompany the parser, navigation, document and CLI boundary.
Release build of `tela_source`, `tela_source_tests` and `tela_source_options_tests`
passed with Visual Studio 2022 on 2026-09-16. This compiles the tests; it does not
execute them. The build explicitly enables MSVC C++ exception unwinding so error
paths release file and window resources.
`git diff --cached --check` and Anatomia `verify --repo .` over the staged diff
passed; all five static gates passed. `anatomia where --repo . --task "source view"`
also identifies the source-view domain.
Tests and real Windows acceptance have not been executed; user authorization is
required for test/start operations. Source-view is not declared operationally
accepted until real rendering and domain-to-viewer launch have been checked.
