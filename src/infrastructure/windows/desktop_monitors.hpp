#pragma once
// @implements SPEC-TL-DESKTOP-OVERLAY
// @spec Desktop overlay
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <tela/desktop_placement.hpp>
#include <vector>

namespace tela::windows {
// Switches the calling thread to Per-Monitor V2 for the scope, so monitor geometry and window
// positions are physical pixels whatever the process default is, and restores it afterwards.
// Windows created inside the scope keep Per-Monitor V2 for their whole life.
class PerMonitorScope {
public:
    PerMonitorScope() noexcept;
    ~PerMonitorScope();
    PerMonitorScope(const PerMonitorScope&) = delete;
    PerMonitorScope& operator=(const PerMonitorScope&) = delete;
    // False when this Windows cannot switch to Per-Monitor V2.
    bool active() const noexcept;
private:
    DPI_AWARENESS_CONTEXT previous_;
};

// Every monitor's work area in desktop physical pixels with its effective DPI. Call inside a
// PerMonitorScope; otherwise Windows reports scaled coordinates.
std::vector<DesktopMonitor> desktop_monitors();
}
