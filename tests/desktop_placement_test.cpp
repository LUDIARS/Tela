// @implements SPEC-TL-DESKTOP-OVERLAY
// @spec Desktop overlay
#include <tela/desktop_placement.hpp>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

namespace {
using tela::DesktopCorner;
using tela::DesktopFrame;
using tela::DesktopMonitor;
using tela::DesktopPlacement;

void require(bool value, const char* why) { if(!value) throw std::runtime_error(why); }
bool near(float a, float b) { return std::abs(a - b) < 1e-3f; }

// Work areas in desktop physical pixels: a 100 % primary, a 150 % monitor to its right and a
// 125 % monitor at a negative origin to its left.
constexpr DesktopMonitor primary{0, 0, 1920, 1040, 1.0f, true};
constexpr DesktopMonitor right_hand{1920, 0, 2560, 1400, 1.5f, false};
constexpr DesktopMonitor left_hand{-1280, 0, 1280, 984, 1.25f, false};
const std::vector<DesktopMonitor> desk{primary, right_hand, left_hand};

DesktopPlacement cornered(DesktopCorner corner, float width = 300, float height = 200, float margin = 16) {
    DesktopPlacement placement;
    placement.width = width; placement.height = height;
    placement.corner = corner; placement.margin_x = placement.margin_y = margin;
    return placement;
}
DesktopPlacement at(int x, int y, DesktopCorner corner = DesktopCorner::bottom_right) {
    auto placement = cornered(corner);
    placement.absolute = true; placement.x = x; placement.y = y;
    return placement;
}
void rejects(const DesktopPlacement& placement, const std::vector<DesktopMonitor>& monitors, const char* why) {
    try { tela::place_on_desktop(placement, monitors); throw std::logic_error(why); }
    catch(const std::invalid_argument&) {}
}
}

int main() {
    try {
        require(tela::desktop_corner_from_name("top-left") == DesktopCorner::top_left, "names map to their corner");
        require(tela::desktop_corner_from_name("bottom-right") == DesktopCorner::bottom_right, "names map to their corner");
        try { tela::desktop_corner_from_name("middle"); throw std::logic_error("unknown corner name accepted"); }
        catch(const std::invalid_argument&) {}
        for(auto corner : {DesktopCorner::top_left, DesktopCorner::top_right, DesktopCorner::bottom_left, DesktopCorner::bottom_right})
            require(tela::desktop_corner_from_name(tela::desktop_corner_name(corner)) == corner, "a stored corner name reads back as that corner");

        // Corners of the primary monitor's work area, the margin away from both edges.
        require(tela::place_on_desktop(cornered(DesktopCorner::bottom_right), desk) == DesktopFrame{1604, 824, 300, 200, 1.0f, false},
            "bottom-right sits the margin inside the primary work area's corner");
        require(tela::place_on_desktop(cornered(DesktopCorner::top_left), desk) == DesktopFrame{16, 16, 300, 200, 1.0f, false},
            "top-left sits the margin inside the primary work area's corner");
        require(tela::place_on_desktop(cornered(DesktopCorner::top_right), desk) == DesktopFrame{1604, 16, 300, 200, 1.0f, false},
            "top-right sits the margin inside the primary work area's corner");
        require(tela::place_on_desktop(cornered(DesktopCorner::bottom_left), desk) == DesktopFrame{16, 824, 300, 200, 1.0f, false},
            "bottom-left sits the margin inside the primary work area's corner");
        require(tela::place_on_desktop(cornered(DesktopCorner::top_left), {right_hand, primary}) == DesktopFrame{16, 16, 300, 200, 1.0f, false},
            "a corner belongs to the monitor flagged primary, not the first one listed");

        // Size and margins are logical and follow the monitor's DPI.
        const DesktopMonitor scaled{0, 0, 1920, 1040, 1.5f, true};
        require(tela::place_on_desktop(cornered(DesktopCorner::bottom_right), {scaled}) == DesktopFrame{1446, 716, 450, 300, 1.5f, false},
            "size and margin are converted with the primary monitor's DPI");

        // An absolute spot keeps its monitor, sized with that monitor's DPI; negative origins are valid.
        require(tela::place_on_desktop(at(2000, 100), desk) == DesktopFrame{2000, 100, 450, 300, 1.5f, false},
            "an absolute spot on a 150 % monitor is sized with that monitor's DPI");
        require(tela::place_on_desktop(at(-1000, 50), desk) == DesktopFrame{-1000, 50, 375, 250, 1.25f, false},
            "an absolute spot at a negative origin is kept");
        require(tela::place_on_desktop(at(1800, 900), {primary}) == DesktopFrame{1620, 840, 300, 200, 1.0f, false},
            "a surface hanging over the work area's edge is pulled back inside it");
        require(tela::place_on_desktop(at(1800, 100), desk) == DesktopFrame{1920, 100, 450, 300, 1.5f, false},
            "a surface straddling two monitors goes to the one holding the larger share of it");

        // A spot on no monitor returns to the same corner of the primary.
        require(tela::place_on_desktop(at(5000, 5000), desk) == DesktopFrame{1604, 824, 300, 200, 1.0f, true},
            "an off-screen surface returns to the primary monitor's same corner");
        require(tela::place_on_desktop(at(5000, 5000, DesktopCorner::top_left), desk) == DesktopFrame{16, 16, 300, 200, 1.0f, true},
            "recovery uses the corner the placement names");

        // A surface larger than the work area is reduced to it rather than left partly off screen.
        require(tela::place_on_desktop(cornered(DesktopCorner::bottom_right, 3000, 2000), desk) == DesktopFrame{0, 0, 1920, 1040, 1.0f, false},
            "an oversized surface is reduced to the work area");
        require(tela::place_on_desktop(cornered(DesktopCorner::top_left, 300, 200, 5000), desk) == DesktopFrame{1620, 840, 300, 200, 1.0f, false},
            "a margin wider than the work area still leaves the surface on screen");

        // Unusable monitors are skipped; nothing usable or an invalid size is an error, not an empty surface.
        require(tela::place_on_desktop(cornered(DesktopCorner::top_left), {{0, 0, 0, 0, 1.0f, true}, primary})
                == DesktopFrame{16, 16, 300, 200, 1.0f, false}, "a monitor without area is ignored");
        rejects(cornered(DesktopCorner::top_left), {}, "no monitor accepted");
        rejects(cornered(DesktopCorner::top_left), {{0, 0, 1920, 1040, 0.0f, true}}, "a monitor without a usable DPI accepted");
        rejects(cornered(DesktopCorner::top_left, 0, 200), desk, "zero width accepted");
        rejects(cornered(DesktopCorner::top_left, 300, std::numeric_limits<float>::quiet_NaN()), desk, "NaN height accepted");
        rejects(cornered(DesktopCorner::top_left, 300, 200, -1), desk, "negative margin accepted");
        rejects(cornered(DesktopCorner::top_left, 20000, 200), desk, "a size no viewport can hold accepted");

        // After a drag the reported placement names where the surface settled and its nearest corner.
        const auto dropped = tela::placement_at(cornered(DesktopCorner::top_right), 100, 700, desk);
        require(dropped.absolute && dropped.x == 100 && dropped.y == 700, "the drop spot is reported as an absolute origin");
        require(dropped.corner == DesktopCorner::bottom_left && near(dropped.margin_x, 100) && near(dropped.margin_y, 140),
            "the nearest corner and the logical distance to it are reported");
        require(tela::place_on_desktop(dropped, desk) == DesktopFrame{100, 700, 300, 200, 1.0f, false},
            "passing the reported placement back restores the spot");
        require(tela::placement_at(dropped, dropped.x, dropped.y, desk) == dropped, "reporting the same spot again changes nothing");

        const auto hanging = tela::placement_at(cornered(DesktopCorner::bottom_left), 1800, -50, {primary});
        require(hanging.x == 1620 && hanging.y == 0 && hanging.corner == DesktopCorner::top_right
                && near(hanging.margin_x, 0) && near(hanging.margin_y, 0),
            "a drop over the edge reports the spot inside the work area it settled at");

        const auto moved = tela::placement_at(cornered(DesktopCorner::top_left), 2400, 1000, desk);
        require(moved.x == 2400 && moved.y == 1000 && moved.corner == DesktopCorner::bottom_left,
            "a drop onto another monitor reports that monitor's nearest corner");
        require(near(moved.margin_x, 320) && near(moved.margin_y, 100.f / 1.5f), "margins are converted back to logical pixels");
        require(tela::place_on_desktop(moved, desk) == DesktopFrame{2400, 1000, 450, 300, 1.5f, false},
            "a spot on a 150 % monitor is restored at that monitor's DPI");
        require(tela::place_on_desktop(moved, {primary}) == DesktopFrame{320, 773, 300, 200, 1.0f, true},
            "once that monitor is gone the surface returns to the same corner of the primary");
        return 0;
    } catch(const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 2;
    }
}
