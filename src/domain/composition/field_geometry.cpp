// @spec SPEC-TL-CONTROLS
#include <tela/field_geometry.hpp>
#include <algorithm>
#include <cmath>

namespace tela {
namespace {
constexpr std::string_view bullet = "\xE2\x80\xA2"; // U+2022, what a secret field shows per character

Rect inner(const Element& e, Rect b) {
    const float p = e.layout.padding;
    return {b.x + p, b.y + p, std::max(0.f, b.width - 2 * p), std::max(0.f, b.height - 2 * p)};
}
struct Line { std::size_t begin, end; };
std::vector<Line> split(std::string_view text) {
    std::vector<Line> lines;
    std::size_t begin = 0;
    for (;;) {
        const auto end = text.find('\n', begin);
        if (end == std::string_view::npos) { lines.push_back({begin, text.size()}); return lines; }
        lines.push_back({begin, end});
        begin = end + 1;
    }
}
std::size_t rows(const Element& e) { return e.kind == ElementKind::text_area ? std::max(1u, e.layout.lines) : 1; }
}

float approximate_text_width(std::string_view text, float font_size) noexcept {
    float width = 0;
    for (std::size_t i = 0; i < text.size(); ++i) {
        const auto c = static_cast<unsigned char>(text[i]);
        if ((c & 0xC0) == 0x80) continue;
        width += c < 0x80 ? 0.55f : 1.f;
    }
    return width * font_size;
}
std::string field_display(const Element& e) {
    if (!e.value.secret) return e.value.text;
    std::string masked;
    const auto count = text_edit::characters(e.value.text, e.value.text.size());
    masked.reserve(count * bullet.size());
    for (std::size_t i = 0; i < count; ++i) masked.append(bullet);
    return masked;
}
std::size_t field_display_offset(const Element& e, std::size_t value_offset) {
    value_offset = text_edit::clamp(e.value.text, value_offset);
    if (!e.value.secret) return value_offset;
    return text_edit::characters(e.value.text, value_offset) * bullet.size();
}
FieldView field_view(const Element& e, Rect bounds, text_edit::Selection selection, bool focused,
                     const Theme& theme, const TextMeasure& measure) {
    FieldView view;
    view.font_size = theme.font_size * e.layout.text_scale;
    view.line_height = theme.line_height * e.layout.text_scale;
    const auto area = inner(e, bounds);
    view.placeholder = e.value.text.empty() && !e.label.empty();
    view.display = view.placeholder ? e.label : field_display(e);
    const std::string_view shown = view.display;
    const auto caret = view.placeholder ? 0 : field_display_offset(e, selection.caret);
    const auto anchor = view.placeholder ? 0 : field_display_offset(e, selection.anchor);
    const auto lines = split(shown);
    const auto caret_line = std::min(text_edit::line_index(shown, caret), lines.size() - 1);
    const auto visible = rows(e);
    view.first_line = caret_line + 1 > visible ? caret_line + 1 - visible : 0;
    const auto& line = lines[caret_line];
    const float caret_x = measure(shown.substr(line.begin, caret - line.begin), view.font_size);
    view.shift = std::max(0.f, caret_x + 1 - area.width);
    for (std::size_t i = view.first_line; i < lines.size() && i < view.first_line + visible; ++i) {
        const Point origin{area.x - view.shift, area.y + static_cast<float>(i - view.first_line) * view.line_height};
        view.lines.push_back({lines[i].begin, lines[i].end, origin});
        const auto start = std::max(std::min(caret, anchor), lines[i].begin);
        const auto end = std::min(std::max(caret, anchor), lines[i].end);
        if (focused && start < end) {
            const float x0 = measure(shown.substr(lines[i].begin, start - lines[i].begin), view.font_size);
            const float x1 = measure(shown.substr(lines[i].begin, end - lines[i].begin), view.font_size);
            view.selection.push_back({origin.x + x0, origin.y, x1 - x0, view.line_height});
        }
    }
    if (focused) {
        const float y = area.y + static_cast<float>(caret_line - view.first_line) * view.line_height;
        view.caret = {area.x - view.shift + caret_x, y, std::max(1.f, view.font_size / 12), view.line_height};
    }
    return view;
}
std::size_t field_caret_at(const Element& e, Rect bounds, text_edit::Selection current, Point point,
                           const Theme& theme, const TextMeasure& measure) {
    if (e.value.text.empty()) return 0;
    const auto view = field_view(e, bounds, current, true, theme, measure);
    const auto area = inner(e, bounds);
    const std::string_view shown = view.display;
    const float row = std::floor((point.y - area.y) / view.line_height);
    const auto index = static_cast<std::size_t>(std::clamp(row, 0.f, static_cast<float>(view.lines.size() - 1)));
    const auto& line = view.lines[index];
    std::size_t best = line.begin;
    float distance = std::abs(point.x - line.origin.x);
    for (auto at = text_edit::next(shown, line.begin); at <= line.end && at > best;
         at = text_edit::next(shown, at)) {
        const float x = line.origin.x + measure(shown.substr(line.begin, at - line.begin), view.font_size);
        if (std::abs(point.x - x) < distance) { distance = std::abs(point.x - x); best = at; }
        if (at == line.end) break;
    }
    if (!e.value.secret) return best;
    return text_edit::byte_offset(e.value.text, text_edit::characters(shown, best));
}
float slider_fraction_at(const Element& e, Rect bounds, float x) noexcept {
    const auto area = inner(e, bounds);
    if (area.width <= 0) return e.value.fraction;
    return std::clamp((x - area.x) / area.width, 0.f, 1.f);
}
}
