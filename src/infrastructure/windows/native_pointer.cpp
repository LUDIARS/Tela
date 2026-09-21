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
    runtime.pointer(event, InputSource::native);
    if(message == WM_LBUTTONDOWN && runtime.captured()) SetCapture(window);
    if(message == WM_LBUTTONUP && GetCapture() == window) ReleaseCapture();
}
}
