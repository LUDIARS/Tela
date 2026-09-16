// @spec Overlay placement
#include "attached_viewport.hpp"
#include "overlay_probe.hpp"
#include <stdexcept>

namespace {
tela::Rect to_rect(const RECT& value) {
    return {static_cast<float>(value.left), static_cast<float>(value.top),
            static_cast<float>(value.right - value.left), static_cast<float>(value.bottom - value.top)};
}
}

void synchronizeAttached(tela::Runtime& runtime, HWND target, tela::Placement placement,
                         float width, float height) {
    // Inside placement is the existing client-area tracking; there is one adapter, not two.
    if(placement == tela::Placement::inside) { synchronizeProbe(runtime, target); return; }
    if(!IsWindow(target)) { runtime.disconnect(); return; }
    RECT window{};
    if(!GetWindowRect(target, &window)) throw std::runtime_error("Cannot read target geometry");
    MONITORINFO monitor{sizeof(monitor)};
    if(!GetMonitorInfoW(MonitorFromWindow(target, MONITOR_DEFAULTTONEAREST), &monitor))
        throw std::runtime_error("Cannot read target monitor");
    const float dpi = GetDpiForWindow(target) / 96.f;
    // The content declares logical pixels; the host contract is desktop physical pixels.
    const auto area = tela::place_viewport(placement, to_rect(window), width * dpi, height * dpi,
                                          to_rect(monitor.rcWork));
    auto next = runtime.viewport();
    const bool visible = IsWindowVisible(target) && !IsIconic(target);
    const bool focused = GetAncestor(GetForegroundWindow(), GA_ROOT) == target;
    const int x = static_cast<int>(area.x), y = static_cast<int>(area.y);
    const int overlay_width = static_cast<int>(area.width), overlay_height = static_cast<int>(area.height);
    if(next.host_id == "attached" && next.desktop_x == x && next.desktop_y == y && next.width == overlay_width
       && next.height == overlay_height && next.dpi_scale == dpi && next.visible == visible && next.focused == focused)
        return;
    ++next.revision;
    next.host_id = "attached"; next.view_id = "beside";
    next.desktop_x = x; next.desktop_y = y;
    next.width = overlay_width; next.height = overlay_height;
    next.dpi_scale = dpi; next.visible = visible; next.focused = focused;
    runtime.viewport(next);
}
