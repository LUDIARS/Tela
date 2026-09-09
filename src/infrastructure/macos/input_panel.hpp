// @spec SPEC-TL-MACOS
#pragma once
#include "window_target.hpp"
#include <tela/runtime.hpp>
#include <exception>

namespace tela::macos {
struct PointerSession {
    Runtime& runtime;
    WindowTarget target;
    std::uint64_t sequence{}, gesture{};
    std::exception_ptr error;
    void dispatch(NSEvent*, PointerPhase) noexcept;
};
NSPanel* make_input_panel(PointerSession&, NSRect);
void close_input_panel(NSPanel*);
}
