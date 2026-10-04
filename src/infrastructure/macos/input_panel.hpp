// @spec SPEC-TL-MACOS
#pragma once
#include "window_target.hpp"
#include <tela/runtime.hpp>
#include <exception>
#include <functional>

namespace tela::macos {
struct PointerSession {
    Runtime& runtime;
    WindowTarget target;
    std::uint64_t sequence{}, gesture{};
    std::exception_ptr error;
    // A surface following a target window takes input only while that application is in front.
    // A desktop surface belongs to no application, so it turns this off.
    bool front_only{true};
    // Sees each native event first; returning true consumes it (the runtime never sees it).
    // The desktop surface uses it for its grip. event.window is the input panel hit.
    std::function<bool(NSEvent*, PointerPhase)> intercept;
    void dispatch(NSEvent*, PointerPhase) noexcept;
};
NSPanel* make_input_panel(PointerSession&, NSRect);
void close_input_panel(NSPanel*);
}
