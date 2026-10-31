// @implements SPEC-TL-LIST-VIEW
// @spec List view
#include <tela/list_view.hpp>
#include <tela/utf8.hpp>
#include <algorithm>

namespace tela {
namespace {
// The backdrop's translucency is the declaration's own colour, as for the desktop probe.
constexpr Color backdrop{12, 14, 20, 166};
constexpr Color edge{255, 255, 255, 64};
constexpr Color alert_band{200, 40, 48, 190};
constexpr Color grip_fill{255, 255, 255, 48};
constexpr Color grip_mark{245, 246, 250, 230};
constexpr float inset = 14, top = 10, grip_size = 28, row_gap = 4;

Layout layout_at(float x, float y, float width, float height = 0) {
    Layout layout;
    layout.positioned = true; layout.x = x; layout.y = y; layout.width = width; layout.height = height;
    layout.padding = 0;
    return layout;
}

float row_height(const Theme& theme) { return theme.line_height + row_gap; }
float header_height(const Theme& theme) { return top + 2 * theme.line_height + row_gap; }

// A row is drawn on one line, so far less than this is ever visible; the cut keeps the label
// under the declaration's 16384-byte limit even for 8 columns of 4096 bytes.
constexpr std::size_t max_row_text_bytes = 8192;

// Cuts on a character boundary so the label stays valid UTF-8.
std::string fit_label(std::string text) {
    if(text.size() <= max_row_text_bytes) return text;
    std::size_t at = 0;
    while(at < text.size()) {
        const auto length = utf8_character_bytes(text, at);
        if(!length || at + length > max_row_text_bytes) break;
        at += length;
    }
    text.resize(at);
    return text;
}

std::string row_text(const ListRow& row) {
    std::string text = row.mark == ListMark::alert ? "! " : row.mark == ListMark::muted ? "? " : "  ";
    for(std::size_t i = 0; i < row.columns.size(); ++i) {
        if(i) text += "   ";
        text += row.columns[i];
    }
    return fit_label(std::move(text));
}

Drawing backdrop_drawing(float width, float height) {
    Drawing drawing;
    drawing.rectangle({0, 0, width, height}, backdrop, {edge, 1}, 12);
    return drawing;
}

Drawing band_drawing(float width, float height) {
    Drawing drawing;
    drawing.rectangle({0, 0, width, height}, alert_band, {{0, 0, 0, 0}, 0}, 4);
    return drawing;
}

Drawing grip_drawing() {
    Drawing drawing;
    drawing.rectangle({0, 0, grip_size, grip_size}, grip_fill, {{0, 0, 0, 0}, 0}, 6);
    for(float y : {9.f, 14.f, 19.f}) drawing.line({8, y}, {grip_size - 8, y}, {grip_mark, 2});
    return drawing;
}
}

float list_view_height(const ListViewFrame& frame, const Theme& theme) {
    // One extra row is kept for the "+N more" line.
    return header_height(theme) + static_cast<float>(frame.visible_rows + 1) * row_height(theme) + inset;
}

Document list_view_document(const ListView& view, const ListViewFrame& frame, const Theme& theme) {
    const float width = frame.width;
    const float height = list_view_height(frame, theme);
    const float text_width = std::max(1.f, width - 2 * inset);
    Document document;
    document.canvas("list.backdrop", backdrop_drawing(width, height), layout_at(0, 0, width, height));
    document.text("list.title", view.title, layout_at(inset, top, std::max(1.f, text_width - grip_size - 8)));
    document.text("list.status", view.status, layout_at(inset, top + theme.line_height, text_width));
    const auto shown = std::min(view.rows.size(), frame.visible_rows);
    float y = header_height(theme);
    for(std::size_t i = 0; i < shown; ++i) {
        const auto& row = view.rows[i];
        const auto id = "list.row." + std::to_string(i);
        if(row.mark == ListMark::alert)
            document.canvas(id + ".band", band_drawing(width - 2 * inset + 8, row_height(theme)),
                            layout_at(inset - 4, y, width - 2 * inset + 8, row_height(theme)));
        document.text(id, row_text(row), layout_at(inset, y + row_gap / 2, text_width));
        y += row_height(theme);
    }
    if(view.rows.size() > shown)
        document.text("list.more", "+" + std::to_string(view.rows.size() - shown) + " more", layout_at(inset, y + row_gap / 2, text_width));
    if(!frame.grip.empty())
        document.canvas(frame.grip, grip_drawing(), layout_at(width - inset - grip_size, top, grip_size, grip_size), InputPolicy::exclusive);
    return document;
}
}
