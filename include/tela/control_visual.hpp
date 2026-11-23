// @implements SPEC-TL-CONTROLS
// @spec Controls
#pragma once
#include <tela/field_geometry.hpp>
#include <tela/layout.hpp>
#include <string>
#include <variant>
#include <vector>

namespace tela {
struct ElementState;
// Backend-neutral look of a control, in logical pixels and painting order. The CPU raster, a
// GPU renderer or any other backend draws these items and decides nothing about the control.
// (The browser measures with its own fonts, so web/tela.js applies the same rules to the frame.)
struct VisualRect {
    Rect rect;
    Color color;
    float border{}; // zero fills; otherwise a border of this logical width inside `rect`
};
struct VisualEllipse { Rect rect; Color color; };
struct VisualText {
    std::string text;   // one unwrapped line
    Point origin;       // top-left of the line box
    Color color;
    Rect clip;          // logical clip in addition to the element's
    float font_size{}, line_height{};
};
using VisualItem = std::variant<VisualRect, VisualEllipse, VisualText>;
struct ControlLook {
    const PlacedElement& placed;
    const ElementState* state;
    bool focused{};
    const Theme& theme;
    const TextMeasure& measure;
};
bool is_control(ElementKind) noexcept;
// Items for a text field, text area, slider, list item, separator, status, scroll or video.
std::vector<VisualItem> control_visual(const ControlLook&);
}
