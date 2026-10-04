// @implements SPEC-TL-DESKTOP-OVERLAY
// @spec Desktop overlay
#include <tela/desktop_points.hpp>
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
using tela::PointDisplay;
using tela::PointFrame;

void require(bool value, const char* why) { if(!value) throw std::runtime_error(why); }
bool near(float a, float b) { return std::abs(a - b) < 1e-3f; }

// AppKit visible frames in global points, bottom-left origin. The primary is a 1440x900 Retina
// display with a 25 pt menu bar and a 70 pt Dock; a 1920x1080 display at 1x sits to its right,
// 200 pt lower, with its own 25 pt menu bar.
constexpr double primary_top = 900;
constexpr PointDisplay retina{0, 70, 1440, 805, 2.0f, true};
constexpr PointDisplay external{1440, -200, 1920, 1055, 1.0f, false};
const std::vector<PointDisplay> desk{retina, external};

DesktopPlacement cornered(DesktopCorner corner, float width = 360, float height = 216, float margin = 16) {
    DesktopPlacement placement;
    placement.width = width; placement.height = height;
    placement.corner = corner; placement.margin_x = placement.margin_y = margin;
    return placement;
}
DesktopPlacement at(int x, int y) {
    auto placement = cornered(DesktopCorner::bottom_right);
    placement.absolute = true; placement.x = x; placement.y = y;
    return placement;
}
void rejects(const DesktopPlacement& placement, const std::vector<PointDisplay>& displays, double top, const char* why) {
    try { tela::place_on_point_desktop(placement, displays, top); throw std::logic_error(why); }
    catch(const std::invalid_argument&) {}
}
}

int main() {
    try {
        // Work areas become top-left-origin points at a DPI scale of 1: sizes are logical points.
        const auto monitors = tela::point_monitors(desk, primary_top);
        require(monitors.size() == 2, "both usable displays become monitors");
        require(monitors[0] == DesktopMonitor{0, 25, 1440, 805, 1.0f, true},
            "the primary's work area starts below the menu bar and ends above the Dock");
        require(monitors[1] == DesktopMonitor{1440, 45, 1920, 1055, 1.0f, false},
            "a lower display measures down from the primary's top edge");

        // Corners of the primary's work area, the margin away from both edges, drawn at 2x.
        require(tela::place_on_point_desktop(cornered(DesktopCorner::bottom_right), desk, primary_top)
                == PointFrame{{1064, 598, 360, 216, 2.0f, false}, 1064, 86, 2128, 1196, 720, 432},
            "bottom-right sits the margin inside the primary work area, at the Retina scale");
        const auto top_left = tela::place_on_point_desktop(cornered(DesktopCorner::top_left), desk, primary_top);
        require(top_left.points == DesktopFrame{16, 41, 360, 216, 2.0f, false},
            "top-left sits the margin below the menu bar");
        require(top_left.appkit_x == 16 && top_left.appkit_y == 900 - 41 - 216,
            "the AppKit frame keeps the bottom-left origin");
        require(top_left.pixel_x == 32 && top_left.pixel_y == 82 && top_left.pixel_width == 720 && top_left.pixel_height == 432,
            "the viewport is the point frame times the backing scale");
        require(tela::place_on_point_desktop(cornered(DesktopCorner::bottom_left, 360, 216, 0), desk, primary_top).appkit_y == 70,
            "a zero bottom margin rests on the Dock's top edge");

        // An absolute spot on the external display is drawn at that display's scale.
        const auto external_spot = tela::place_on_point_desktop(at(2000, 300), desk, primary_top);
        require(external_spot.points == DesktopFrame{2000, 300, 360, 216, 1.0f, false},
            "an absolute spot on the 1x display stays put");
        require(external_spot.pixel_x == 2000 && external_spot.pixel_width == 360,
            "a 1x display draws one pixel per point");
        require(external_spot.appkit_y == 900 - 300 - 216, "the AppKit origin follows the top-left spot");
        // Straddling the edge: the display holding the larger share takes it and clamps it in.
        const auto straddling = tela::place_on_point_desktop(at(1300, 300), desk, primary_top);
        require(straddling.points.x == 1440 && near(straddling.points.dpi_scale, 1),
            "a surface mostly on the external display moves onto it and takes its scale");
        const auto mostly_primary = tela::place_on_point_desktop(at(1200, 300), desk, primary_top);
        require(mostly_primary.points.x == 1440 - 360 && near(mostly_primary.points.dpi_scale, 2),
            "a surface mostly on the primary is clamped into it at 2x");
        // Off every display: back to the primary's corner, marked as recovered.
        const auto lost = tela::place_on_point_desktop(at(-5000, 300), desk, primary_top);
        require(lost.points.recovered && lost.points.x == 1064 && lost.points.y == 598,
            "a spot on no display returns to the primary's corner");
        // A surface larger than the work area is reduced to it.
        const auto huge = tela::place_on_point_desktop(cornered(DesktopCorner::top_left, 4000, 4000), desk, primary_top);
        require(huge.points == DesktopFrame{0, 25, 1440, 805, 2.0f, false}, "an oversized surface fills the primary work area");
        // A display whose scale changes (Retina mode switched) redraws at the new scale.
        const std::vector<PointDisplay> rescaled{{0, 70, 1440, 805, 1.0f, true}, external};
        require(tela::place_on_point_desktop(cornered(DesktopCorner::bottom_right), rescaled, primary_top).pixel_width == 360,
            "the viewport follows the backing scale of the moment");

        // Fractional work-area edges round to whole points without opening a gap.
        const std::vector<PointDisplay> fractional{{0.4, 70.4, 100.2, 50.2, 1.0f, true}};
        require(tela::point_monitors(fractional, 200) == std::vector<DesktopMonitor>{{0, 79, 101, 51, 1.0f, true}},
            "edges round independently");

        // Displays the host cannot use are ignored; none left is an error.
        constexpr double nan = std::numeric_limits<double>::quiet_NaN();
        const std::vector<PointDisplay> with_broken{retina, {0, 0, 0, 100, 1.0f, false}, {nan, 0, 10, 10, 1.0f, false},
                                                    {3360, 0, 100, 100, 0.0f, false}, {3360, 0, 100, 100, 9.0f, false}};
        require(tela::point_monitors(with_broken, primary_top).size() == 1, "empty, non-finite and unscaled displays are ignored");
        rejects(cornered(DesktopCorner::top_left), {{0, 0, 100, 100, 0.0f, true}}, primary_top,
                "a desk without usable displays was accepted");
        rejects(cornered(DesktopCorner::top_left), {}, primary_top, "an empty desk was accepted");
        rejects(cornered(DesktopCorner::top_left, 0), desk, primary_top, "a zero width was accepted");
        rejects(cornered(DesktopCorner::top_left, 100, 100, -1), desk, primary_top, "a negative margin was accepted");
        rejects(cornered(DesktopCorner::top_left), desk, nan, "a non-finite primary top was accepted");

        // After a drag the report names the nearest corner of that display and margins in points,
        // and passing it back restores the spot.
        const auto dropped = tela::point_placement_at(cornered(DesktopCorner::top_left), 1000, 500, desk, primary_top);
        require(dropped.absolute && dropped.x == 1000 && dropped.y == 500, "the drop is reported where it settled");
        require(dropped.corner == DesktopCorner::bottom_right && near(dropped.margin_x, 80) && near(dropped.margin_y, 114),
            "the nearest corner and its distance are in points");
        require(tela::place_on_point_desktop(dropped, desk, primary_top).points == DesktopFrame{1000, 500, 360, 216, 2.0f, false},
            "the reported placement restores the spot");
        require(tela::point_placement_at(dropped, dropped.x, dropped.y, desk, primary_top) == dropped,
            "reporting the restored spot gives the same placement");
        const auto unplugged = tela::place_on_point_desktop(tela::point_placement_at(dropped, 2000, 300, desk, primary_top),
                                                            {retina}, primary_top);
        require(unplugged.points.recovered && near(unplugged.points.dpi_scale, 2),
            "a spot on an unplugged display returns to the primary at its scale");
        std::cout << "tela desktop point placement contract passed\n";
        return 0;
    } catch(const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
