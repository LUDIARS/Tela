// @implements SPEC-TL-PLACEMENT
// @spec Overlay placement
#pragma once
#include <tela/geometry.hpp>
#include <string_view>

namespace tela {
// Where the overlay's viewport sits relative to the host window.
enum class Placement { inside, left, right, above, below };

// Reads a placement name. The mapping lives with the type so callers pass the text through
// rather than repeating the values.
Placement placement_from_name(std::string_view);

// Places a viewport of the requested size beside the host window, kept inside the work area.
// `inside` returns the host rect itself so callers keep one code path. When the chosen side
// has no room the opposite side is used, and a size larger than the work area is reduced to it.
Rect place_viewport(Placement, Rect host, float width, float height, Rect work_area);
}
