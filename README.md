# Tela (Tl)

C++ declarative tool UI library. Product UX: **LUDIARSの柔軟なツール描画**.
The first host adapter is a Windows overlay for Unity's Scene view; Orbis and
Iter can reuse the same C++ contracts without depending on Unity.

## Components

| Target | Responsibility |
|---|---|
| `Tela::Core` | Stable-ID declarations, column/explicit-width row layout, clipped hit regions, retained button state, theme, invalidation, gesture and bridge contracts |
| `Tela::Pictor` | Explicit CPU TrueType/premultiplied bitmap renderer using existing Pictor |
| `Tela::Windows` | Transparent/nonactivating overlay, exclusive region windows, current-user local pipe |
| `Tela::Transitions` | Sample transition data, editing declarations and persistence |

`unity/com.ludiars.tela` is an Editor-only UPM package. It sends Scene geometry,
anchors, selection and observed input; native Tela renders the UI. The example
host includes a modeless Windows text editor for transition source/target/condition.
The wire format and the transition file are versioned, bounded UTF-8 formats.

C++ library/executable builds, four library contract tests and Unity 6 reference
assembly compilation have been checked. Native visual/input/load acceptance is
tracked separately in [the composition procedure](spec/windows-composition.md).
No claim of transparent Vulkan presentation, complex-script shaping, arbitrary
widget completeness or production Unity performance is made.

## Build

Requires CMake 3.20+ and a C++20 compiler.

```sh
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Consumers link `Tela::Core` and include `tela/document.hpp`.
Install with `cmake --install build --config Release --prefix <prefix>`;
consumers use `find_package(Tela CONFIG REQUIRED)` and `Tela::Core`.
`tests/consumer` is an independent CMake consumer example.

For Windows, enable `-DTELA_BUILD_WINDOWS=ON` and provide
`-DTELA_PICTOR_INCLUDE_DIR=<pictor include>` and
`-DTELA_PICTOR_LIBRARY=<matching compiler/config pictor.lib>`.
No sibling source checkout is hard-coded or silently fetched. Supply a licensed
TrueType font with `tela_overlay --font <file.ttf>`; it is not bundled.
Start `tela` / `tela-probe-target` / `tela-probe` through Excubitor from the main
project following the procedure above. No test service autostarts.

For Unity reference compilation without launching the Editor:
`scripts/verify-unity.ps1 -EditorData <installed Unity 6 Editor/Data>`.
Actual Scene alignment, input coexistence and reconnect still require a live Editor.

See [UX](spec/ux/product.md), [DDD](spec/architecture/ddd.md),
[architecture](spec/architecture.md), [contracts](spec/contracts.md) and
[IPC protocol](spec/bridge-protocol.md).
