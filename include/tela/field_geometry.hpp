// @implements SPEC-TL-CONTROLS
// @spec Controls
#pragma once
#include <tela/document.hpp>
#include <tela/text_edit.hpp>
#include <functional>
#include <string_view>
#include <vector>

namespace tela {
// Logical width of `text` drawn at `font_size` logical pixels. Hosts with a font supply the
// real advance; without one the runtime uses approximate_text_width.
using TextMeasure = std::function<float(std::string_view text, float font_size)>;
// 0.55 em per ASCII character and 1 em otherwise: enough to place a caret without a font.
float approximate_text_width(std::string_view text, float font_size) noexcept;

struct FieldLine {
    std::size_t begin{}, end{}; // byte range of FieldView::display
    Point origin;               // logical top-left of the line's text
};
// What a text field or area shows. The rule (no soft wrap, horizontal shift that keeps the
// caret visible, first row that keeps the caret's line visible) is the same for every backend.
struct FieldView {
    std::string display;   // value, bullets for a secret, or the placeholder
    bool placeholder{};
    std::vector<FieldLine> lines;
    Rect caret;            // logical; width zero when the caret is not shown
    std::vector<Rect> selection;
    float font_size{}, line_height{}, shift{};
    std::size_t first_line{};
};
std::string field_display(const Element&);
// Maps a value byte offset to the same character in field_display.
std::size_t field_display_offset(const Element&, std::size_t value_offset);
FieldView field_view(const Element&, Rect bounds, text_edit::Selection, bool focused,
                     const Theme&, const TextMeasure&);
// Value byte offset nearest to the logical point, given the selection the field shows now.
std::size_t field_caret_at(const Element&, Rect bounds, text_edit::Selection current, Point,
                           const Theme&, const TextMeasure&);
// Slider fraction under the logical x coordinate.
float slider_fraction_at(const Element&, Rect bounds, float x) noexcept;
}
