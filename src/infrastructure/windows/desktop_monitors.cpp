// @spec SPEC-TL-DESKTOP-OVERLAY
#include "desktop_monitors.hpp"
#include <ShellScalingApi.h>

namespace tela::windows {
namespace {
constexpr float default_dpi = 96.f;

BOOL CALLBACK collect(HMONITOR monitor, HDC, LPRECT, LPARAM data) {
    auto& monitors = *reinterpret_cast<std::vector<DesktopMonitor>*>(data);
    MONITORINFO info{sizeof(info)};
    // A monitor that disappears while being enumerated is simply not offered.
    if(!GetMonitorInfoW(monitor, &info)) return TRUE;
    UINT dpi_x{}, dpi_y{};
    // Without an effective DPI the monitor cannot be sized correctly; leaving it out lets the
    // placement fall back to a monitor it can size instead of drawing at a guessed scale.
    if(FAILED(GetDpiForMonitor(monitor, MDT_EFFECTIVE_DPI, &dpi_x, &dpi_y)) || !dpi_x) return TRUE;
    monitors.push_back({info.rcWork.left, info.rcWork.top, info.rcWork.right - info.rcWork.left,
                        info.rcWork.bottom - info.rcWork.top, static_cast<float>(dpi_x) / default_dpi,
                        (info.dwFlags & MONITORINFOF_PRIMARY) != 0});
    return TRUE;
}
}

PerMonitorScope::PerMonitorScope() noexcept
    : previous_(SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2)) {}
PerMonitorScope::~PerMonitorScope() { if(previous_) SetThreadDpiAwarenessContext(previous_); }
bool PerMonitorScope::active() const noexcept { return previous_ != nullptr; }

std::vector<DesktopMonitor> desktop_monitors() {
    std::vector<DesktopMonitor> monitors;
    EnumDisplayMonitors(nullptr, nullptr, collect, reinterpret_cast<LPARAM>(&monitors));
    return monitors;
}
}
