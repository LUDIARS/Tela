// @implements SPEC-TL-PLACEMENT
// @spec Overlay placement
#include <tela/placement.hpp>
#include <iostream>
#include <stdexcept>

namespace {
void require(bool value, const char* why) { if(!value) throw std::runtime_error(why); }
constexpr tela::Rect work{0, 0, 1920, 1080};
constexpr tela::Rect host{800, 300, 400, 300};
}

int main() {
    try {
        require(tela::placement_from_name("inside") == tela::Placement::inside, "names map to their placement");
        require(tela::placement_from_name("right") == tela::Placement::right, "names map to their placement");
        require(tela::placement_from_name("below") == tela::Placement::below, "names map to their placement");
        try { tela::placement_from_name("sideways"); throw std::logic_error("unknown placement name accepted"); }
        catch(const std::invalid_argument&) {}

        require(tela::place_viewport(tela::Placement::inside, host, 500, 400, work) == host,
            "inside keeps the host rect so callers need no branch");
        require(tela::place_viewport(tela::Placement::right, host, 500, 400, work) == tela::Rect{1200, 300, 500, 400},
            "right sits past the host's right edge at the requested size");
        require(tela::place_viewport(tela::Placement::left, host, 500, 400, work) == tela::Rect{300, 300, 500, 400},
            "left sits before the host's left edge");
        require(tela::place_viewport(tela::Placement::above, host, 500, 200, work) == tela::Rect{800, 100, 500, 200},
            "above sits over the host's top edge");
        require(tela::place_viewport(tela::Placement::below, host, 500, 200, work) == tela::Rect{800, 600, 500, 200},
            "below sits under the host's bottom edge");

        // A side without room flips instead of covering the host.
        const tela::Rect near_right{1500, 300, 400, 300};
        require(tela::place_viewport(tela::Placement::right, near_right, 500, 400, work) == tela::Rect{1000, 300, 500, 400},
            "right flips to the left when the work area has no room");
        const tela::Rect near_left{100, 300, 400, 300};
        require(tela::place_viewport(tela::Placement::left, near_left, 500, 400, work) == tela::Rect{500, 300, 500, 400},
            "left flips to the right when the work area has no room");

        // Neither side fitting still stays on screen, and an oversized view is reduced to the work area.
        const tela::Rect wide{0, 0, 1920, 1080};
        require(tela::place_viewport(tela::Placement::right, wide, 3000, 2000, work) == tela::Rect{0, 0, 1920, 1080},
            "an oversized view is reduced to the work area and clamped on screen");

        // A degenerate request falls back to the host rather than presenting nothing.
        require(tela::place_viewport(tela::Placement::right, host, 0, 400, work) == host, "zero width falls back to the host");
        require(tela::place_viewport(tela::Placement::right, host, 500, 400, {0, 0, 0, 0}) == host,
            "an unusable work area falls back to the host");
        return 0;
    } catch(const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 2;
    }
}
