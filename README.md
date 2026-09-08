# Tela (Tl)

C++ declarative UI framework for overlays on creative applications. Pictor is the
planned rendering backend. The first host is Unity Editor's Scene view.

## Bootstrap status

Implemented: GPU-independent panel/text/button document builder with stable IDs,
exception rollback, host geometry/input/selection value types, declaration example,
and a CTest contract test. No external runtime dependencies are needed to build.

Not implemented: layout, text shaping, action dispatch, retained UI reconciliation,
Pictor adapter, native transparent window, IPC transport, or Unity Editor package.
The example constructs a document; it does **not** display an overlay.

## Build

Requires CMake 3.20+ and a C++20 compiler.

```sh
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Consumers link `Tela::Core` and include `tela/document.hpp`.
See [architecture](spec/architecture.md) and [contracts](spec/contracts.md).
