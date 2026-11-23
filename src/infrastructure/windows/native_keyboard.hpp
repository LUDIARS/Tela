#pragma once
// @implements SPEC-TL-CONTROLS
// @spec Controls
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <tela/runtime.hpp>
#include <cstdint>

namespace tela::windows {
// Turns WM_KEYDOWN and WM_CHAR into runtime key and text events. WM_CHAR also carries an
// IME's committed string (DefWindowProc turns WM_IME_CHAR into it), so composition is native.
class NativeKeyboard {
public:
    // True when the runtime used the key; the window may give unused keys their own meaning.
    bool key(Runtime&, WPARAM virtual_key);
    bool character(Runtime&, WPARAM code_unit);
private:
    std::uint64_t sequence_{};
    wchar_t high_surrogate_{};
};
std::uint32_t current_modifiers() noexcept;
}
