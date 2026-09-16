// @spec SPEC-TL-SOURCE-VIEW
#include <tela/source_view.hpp>
#include <algorithm>
#include <cmath>

namespace tela {
namespace {
constexpr float gutter = 88;
float toolbar_height(const Theme& theme) { return theme.line_height * 4 + 28; }
Layout source_box(float x, float y, float width, float height) {
    return {.width = std::clamp(width, 1.f, 32768.f), .height = height,
        .padding = 0, .positioned = true, .x = x, .y = y};
}
bool is_visible(const Viewport& view) {
    return view.visible && view.width > 0 && view.height > 0 && std::isfinite(view.dpi_scale) && view.dpi_scale > 0;
}
}
std::size_t source_page_rows(const Viewport& view, const Theme& theme) {
    if(!is_visible(view) || !std::isfinite(theme.line_height) || theme.line_height <= 0) return 1;
    return static_cast<std::size_t>(std::clamp(std::floor((view.height / view.dpi_scale - toolbar_height(theme)) /
        theme.line_height), 1.f, 200.f));
}
std::size_t source_page_columns(const Viewport& view, const Theme& theme) {
    if(!is_visible(view)) return 1;
    return static_cast<std::size_t>(std::clamp(std::floor((view.width / view.dpi_scale - gutter - 12) /
        std::max(1.f, theme.font_size)), 1.f, 2048.f));
}
Document source_document(const SourceView& source, const Viewport& view, const Theme& theme,
    std::function<void(SourceAction)> navigate) {
    Document doc;
    if(!is_visible(view)) return doc;
    const float width = view.width / view.dpi_scale;
    const auto rows = source_page_rows(view, theme);
    const auto last = std::min(source.line_count(), source.first_line() + rows - 1);
    doc.text("source/path", source.name(), source_box(8, 4, width - 16, theme.line_height));
    doc.text("source/status", "Lines " + std::to_string(source.first_line()) + "-" + std::to_string(last) +
        " / " + std::to_string(source.line_count()) + "   Column offset " + std::to_string(source.column()) +
        "   Target " + std::to_string(source.target_line()), source_box(8, theme.line_height + 8, width - 16, theme.line_height));
    const SourceAction actions[]{SourceAction::previous, SourceAction::next, SourceAction::left,
        SourceAction::right, SourceAction::first, SourceAction::target};
    const char* labels[]{"Prev page", "Next page", "Left", "Right", "First", "Target"};
    for(std::size_t i = 0; i < 6; ++i) {
        const auto action = actions[i];
        doc.button("source/nav/" + std::to_string(i), labels[i], [navigate, action] { if(navigate) navigate(action); },
            source_box(8 + static_cast<float>(i % 3) * ((width - 16) / 3),
                theme.line_height * (2 + static_cast<float>(i / 3)) + 16,
                (width - 16) / 3 - 4, theme.line_height));
    }
    // Conservative width for mixed-width scripts. Horizontal navigation never splits UTF-8.
    const auto columns = source_page_columns(view, theme);
    for(auto line = source.first_line(); line <= last; ++line) {
        const float y = toolbar_height(theme) + static_cast<float>(line - source.first_line()) * theme.line_height;
        if(line == source.target_line()) {
            Drawing highlight;
            highlight.rectangle({0, 0, std::max(1.f, width), theme.line_height}, {50, 65, 88, 255});
            doc.canvas("source/target", std::move(highlight), source_box(0, y, width, theme.line_height));
        }
        doc.text("source/number/" + std::to_string(line), std::to_string(line), source_box(8, y, gutter - 16, theme.line_height));
        doc.text("source/line/" + std::to_string(line), source.slice(line, columns),
            source_box(gutter, y, width - gutter - 8, theme.line_height));
    }
    return doc;
}
}
