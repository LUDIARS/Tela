// @implements SPEC-TL-DESKTOP-OVERLAY
// @spec Desktop overlay
#pragma once
#include <tela/desktop_placement.hpp>
#include <vector>

namespace tela {
// One display as a point-based desktop (macOS) reports it: the work area (AppKit visibleFrame)
// in global points with a bottom-left origin, and the display's backing scale (pixels per point).
struct PointDisplay {
    double x{}, y{}, width{}, height{};
    float backing_scale{1};
    bool primary{};
    bool operator==(const PointDisplay&) const = default;
};

// A surface placed on a point-based desktop. There is no desktop-wide pixel grid there, so the
// placement is solved in points and the pixels follow from the backing scale of the display the
// surface lands on.
struct PointFrame {
    // Top-left-origin global points measured down from the primary display's top edge;
    // dpi_scale is the backing scale of the display holding the surface.
    DesktopFrame points;
    // The same rectangle with AppKit's bottom-left origin, for the window frame.
    double appkit_x{}, appkit_y{};
    // The viewport: global points times the backing scale, the pixels the surface is drawn in.
    int pixel_x{}, pixel_y{}, pixel_width{}, pixel_height{};
    bool operator==(const PointFrame&) const = default;
};

// The displays as monitors in top-left-origin points. Sizes and margins of a DesktopPlacement
// are logical points, so every monitor has a DPI scale of 1 here; `primary_top` is the top edge
// (NSMaxY of the full frame) of the primary display. Work-area edges are rounded to whole points.
// Displays with no area, coordinates beyond ±1000000 or a backing scale outside 0.25..8 are left
// out, as place_on_desktop would ignore them.
std::vector<DesktopMonitor> point_monitors(const std::vector<PointDisplay>&, double primary_top);

// place_on_desktop on a point-based desktop: corners and margins are in points, an absolute
// placement's (x, y) is the top-left in global points (top-left origin), and the result carries
// the backing scale and pixel viewport of the display that holds the surface. Throws
// std::invalid_argument as place_on_desktop does, including when no display is usable.
PointFrame place_on_point_desktop(const DesktopPlacement&, const std::vector<PointDisplay>&, double primary_top);

// placement_at on a point-based desktop: the placement to report after the surface was dragged so
// its top-left is at (x, y) global points, with its margins in points.
DesktopPlacement point_placement_at(const DesktopPlacement& current, int x, int y,
                                    const std::vector<PointDisplay>&, double primary_top);
}
