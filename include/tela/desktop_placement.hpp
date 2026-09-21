// @implements SPEC-TL-DESKTOP-OVERLAY
// @spec Desktop overlay
#pragma once
#include <string_view>
#include <vector>

namespace tela {
// Corner of a monitor's work area that a desktop surface is anchored to.
enum class DesktopCorner { top_left, top_right, bottom_left, bottom_right };

// Reads `top-left`, `top-right`, `bottom-left` or `bottom-right`. The mapping lives with the
// type so callers pass their setting through rather than repeating the values.
DesktopCorner desktop_corner_from_name(std::string_view);
// The name desktop_corner_from_name reads back, for callers that store a placement as text.
const char* desktop_corner_name(DesktopCorner) noexcept;

// One monitor as the host reads it: the work area in desktop physical pixels (negative origins
// are valid) and that monitor's own DPI scale.
struct DesktopMonitor {
    int x{}, y{}, width{}, height{};
    float dpi_scale{1};
    bool primary{};
    bool operator==(const DesktopMonitor&) const = default;
};

// Where a surface with no target window sits. Size and margins are logical pixels, converted
// with the DPI of the monitor the surface lands on. Without `absolute` the surface sits in
// `corner` of the primary monitor's work area, `margin_x`/`margin_y` away from its edges. With
// `absolute`, (x, y) is the top-left in desktop physical pixels, and `corner` and the margins
// only say where the surface returns once that spot is on no monitor any more.
struct DesktopPlacement {
    float width{}, height{};
    DesktopCorner corner{DesktopCorner::bottom_right};
    float margin_x{16}, margin_y{16};
    bool absolute{};
    int x{}, y{};
    bool operator==(const DesktopPlacement&) const = default;
};

// The surface's rectangle in desktop physical pixels and the DPI scale it is drawn at.
struct DesktopFrame {
    int x{}, y{}, width{}, height{};
    float dpi_scale{1};
    bool recovered{}; // the absolute spot was on no monitor, so it returned to the primary's corner
    bool operator==(const DesktopFrame&) const = default;
};

// Places a surface on the desktop. An absolute surface belongs to the monitor whose work area
// holds the largest share of it, sized with that monitor's DPI; when no monitor holds any of it,
// it returns to the same corner of the primary monitor. The result always lies inside one work
// area, and a surface larger than the work area is reduced to it. Monitors with no area or an
// unusable DPI are ignored. Throws std::invalid_argument for a size or margin that is not a
// finite non-negative number (the size must also be positive) or when no usable monitor is left.
DesktopFrame place_on_desktop(const DesktopPlacement&, const std::vector<DesktopMonitor>&);

// The placement to report after the surface was moved so its top-left is at (x, y). It is the
// absolute spot the surface settles at, with the corner of its monitor it is now nearest to and
// its logical distance from that corner, so passing it back restores the spot and a lost monitor
// returns it to that corner of the primary. The size is taken from `current`.
DesktopPlacement placement_at(const DesktopPlacement& current, int x, int y,
                              const std::vector<DesktopMonitor>&);
}
