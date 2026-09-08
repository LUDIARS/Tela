#pragma once
#include <tela/document.hpp>

namespace tela {
struct PlacedElement {
    Element element;
    Rect bounds, clip;
};
std::vector<PlacedElement> arrange(const Document&, float width, float height, const Theme&);
}
