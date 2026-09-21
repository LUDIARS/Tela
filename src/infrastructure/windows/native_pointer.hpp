#pragma once
// @implements SPEC-TL-INPUT
// @spec Input ownership
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <tela/runtime.hpp>
#include <cstdint>

namespace tela::windows {
// Turns the primary button and moves a Tela window receives into native runtime input, and
// holds capture while the runtime keeps the gesture. Every Tela window that takes input goes
// through this, so there is one way the desktop's clicks reach a declaration.
class NativePointer {
public:
    static bool handles(UINT message) noexcept;
    void forward(Runtime&, HWND window, UINT message, LPARAM position);
private:
    std::uint64_t sequence_{}, gesture_{};
};
}
