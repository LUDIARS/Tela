// @spec SPEC-TL-DESKTOP-OVERLAY
#pragma once
#include <tela/desktop_points.hpp>
#include <vector>

namespace tela::macos {
// The attached displays as a desktop surface places itself on them.
struct DesktopScreens {
    std::vector<PointDisplay> displays; // empty while no display is attached
    double primary_top{};               // NSMaxY of the primary (menu bar) display's full frame
};
// Reads every NSScreen's visibleFrame and backingScaleFactor. The first screen is the primary:
// AppKit lists the display with the menu bar first, and global coordinates start from it.
// Must run on the AppKit main thread.
DesktopScreens desktop_screens();
}
