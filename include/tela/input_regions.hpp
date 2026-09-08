#pragma once
#include <tela/layout.hpp>

namespace tela {
struct InputRegion { std::string id; Rect bounds; std::vector<Rect> fragments; };
// Effective exclusive regions after later shared/exclusive declarations occlude them.
std::vector<InputRegion> exclusive_regions(const std::vector<PlacedElement>&);
}
