#pragma once
#include <tela/document.hpp>
#include <string>
#include <unordered_map>

namespace tela {
struct PlacedElement {
    Element element;
    Rect bounds, clip;
    // Scroll containers: applied offset (clamped to the content) and the children's height.
    float scroll{}, content_height{};
};
// Requested scroll offsets by container ID; arrange clamps them to [0, content - visible].
using ScrollOffsets = std::unordered_map<std::string, float>;
std::vector<PlacedElement> arrange(const Document&, float width, float height, const Theme&,
                                   const ScrollOffsets& offsets = {});
}