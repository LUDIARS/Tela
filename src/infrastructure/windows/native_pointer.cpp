// @spec SPEC-TL-INPUT
#include "native_pointer.hpp"
#include <windowsx.h>

namespace tela::windows {
bool NativePointer::handles(UINT message) noexcept {
    return message == WM_LBUTTONDOWN || message == WM_LBUTTONUP || message == WM_MOUSEMOVE;
}

void NativePointer::forward(Runtime& runtime, HWND window, UINT message, LPARAM position) {
    POINT desktop{GET_X_LPARAM(position), GET_Y_LPARAM(position)};
    ClientToScreen(window, &desktop);
    if(message == WM_LBUTTONDOWN) ++gesture_;
    HostPointerEvent event;
    event.sequence = ++sequence_;
    event.viewport_revision = runtime.viewport().revision;
    event.gesture_id = gesture_;
    event.phase = message == WM_LBUTTONDOWN ? PointerPhase::down
                : message == WM_LBUTTONUP ? PointerPhase::up : PointerPhase::move;
    event.button = message == WM_MOUSEMOVE ? PointerButton::none : PointerButton::primary;
    event.desktop_x = desktop.x;
    event.desktop_y = desktop.y;
    // Shift extends a text selection from the caret.
    if(GetKeyState(VK_SHIFT) < 0) event.modifiers |= modifier_shift;
    if(GetKeyState(VK_CONTROL) < 0) event.modifiers |= modifier_control;
    runtime.pointer(event, InputSource::native);
    if(message == WM_LBUTTONDOWN && runtime.captured()) SetCapture(window);
    if(message == WM_LBUTTONUP && GetCapture() == window) ReleaseCapture();
}

void NativePointer::wheel(Runtime& runtime, UINT message, WPARAM delta, LPARAM position, std::uint32_t modifiers) {
    HostPointerEvent event;
    event.sequence = ++sequence_;
    event.viewport_revision = runtime.viewport().revision;
    event.gesture_id = gesture_;
    event.phase = PointerPhase::wheel;
    event.desktop_x = GET_X_LPARAM(position);
    event.desktop_y = GET_Y_LPARAM(position);
    const float notches = static_cast<float>(GET_WHEEL_DELTA_WPARAM(delta)) / WHEEL_DELTA;
    // WM_MOUSEHWHEEL is positive to the right; the contract's wheel_x has no direction preference.
    if(message == WM_MOUSEHWHEEL) event.wheel_x = notches; else event.wheel_y = notches;
    event.modifiers = modifiers;
    runtime.pointer(event, InputSource::native);
}
}
