// @spec SPEC-TL-SPEC-VIEW
#include <tela/spec_view.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <optional>
#include <stdexcept>
#include <utility>

namespace tela {
namespace {
constexpr std::size_t shapes_per_canvas = 200; // Drawing accepts at most 256 commands
constexpr float max_layout_extent = 32768;     // Document layout dimension limit
constexpr float page_rows = 4;                 // position line and three page buttons
constexpr float card_padding = 6, outline_width = 1.5f, corner_radius = 6;
constexpr float code_offset = 4, title_offset = 26;
constexpr float tolerance = .5f; // exported coordinates are whole or half pixels
// Pf sizes the cards for the default 16 px theme, whose line is 24 px. The title takes the
// rows left under the code line, so a long title wraps instead of losing everything past
// the first break.
// Groups are told apart by outline color; the palette repeats beyond six groups.
// Kept in the same order as Pf's SPEC_VIEW_PALETTE so both views draw the same picture.
constexpr std::array<Color, 6> group_palette{{
    {120, 200, 255, 255}, {255, 196, 90, 255}, {150, 230, 150, 255},
    {255, 140, 170, 255}, {200, 160, 255, 255}, {240, 240, 120, 255}}};

std::string group_prefix(const std::string& group_id) { return "spec-view/group/" + group_id; }
float layout_extent(float value) { return std::clamp(value, 1.0f, max_layout_extent); }

std::string card_heading(const SpecViewCard& card) {
    std::string heading = card.code;
    if(!card.status.empty()) heading += " " + card.status;
    return heading + " v" + std::to_string(card.version);
}

// Where the picture lands in a viewport and which exported rows it shows.
struct Frame {
    float width{}, height{}, strip{};
    Rect area;           // the whole picture, its top shifted by the scroll position
    float scale{};
    float top{}, page{}; // shown exported rows: [top, top + page]
    bool scrolls{};
};

std::optional<Frame> view_frame(const SpecViewInfo& info, const Viewport& viewport, float scroll) {
    // A hidden or degenerate host declares nothing, matching the overlay lifecycle contract.
    if(!viewport.visible || viewport.width <= 0 || viewport.height <= 0 || !(viewport.dpi_scale > 0)) return std::nullopt;
    Frame f;
    f.width = viewport.width / viewport.dpi_scale;
    f.height = viewport.height / viewport.dpi_scale;
    f.strip = std::min(spec_view_controls_width, f.width * .35f);
    f.area = spec_view_area(info, {f.strip, 0, f.width - f.strip, f.height});
    if(f.area.width <= 0 || f.area.height <= 0) return std::nullopt;
    f.scale = f.area.width / info.width;
    f.scrolls = f.area.height > f.height + tolerance;
    f.page = f.scrolls ? f.height / f.scale : info.height;
    f.top = f.scrolls ? scroll : 0;
    f.area.y -= f.top * f.scale;
    return f;
}

// Paged views draw whole cards only; a card taller than the page shows from its top.
bool card_shown(const Rect& b, const Frame& f) {
    if(!f.scrolls) return true;
    if(b.y < f.top - tolerance) return false;
    return b.y + b.height <= f.top + f.page + tolerance || (b.height > f.page && b.y <= f.top + tolerance);
}

// Counts the visible groups' cards in Pf's order, e.g. "Cards 4-6 / 50".
std::string page_position(const SpecView& view, const Frame& f) {
    std::size_t total = 0, first = 0, last = 0;
    for(const auto& group : view.groups()) {
        if(!group.visible) continue;
        for(const auto& card : view.cards()) {
            if(card.group_id != group.id) continue;
            ++total;
            if(!card_shown(card.bounds, f)) continue;
            if(!first) first = total;
            last = total;
        }
    }
    // A page holding one card reads "Cards 50 / 50", not "Cards 50-50 / 50".
    const std::string shown = !first ? std::string("0")
        : first == last ? std::to_string(first) : std::to_string(first) + "-" + std::to_string(last);
    return "Cards " + shown + " / " + std::to_string(total);
}

void declare_page_controls(Document& document, const std::string& position,
    const std::function<void(SpecViewAction)>& navigate, const Layout& row) {
    document.text("spec-view/page/position", position, row);
    const std::array<std::pair<SpecViewAction, const char*>, 3> actions{{
        {SpecViewAction::previous, "Prev page"}, {SpecViewAction::next, "Next page"}, {SpecViewAction::first, "First"}}};
    for(std::size_t i = 0; i < actions.size(); ++i) {
        const auto action = actions[i].first;
        document.button("spec-view/page/" + std::to_string(i), actions[i].second,
            [navigate, action] { if(navigate) navigate(action); }, row);
    }
}

// The left-hand strip: title, one toggle per group and, when paged, the page controls.
void declare_controls(Document& document, const SpecView& view, const Frame& f, const Theme& theme,
    const std::function<void(const std::string&)>& toggle, const std::function<void(SpecViewAction)>& navigate) {
    Layout controls;
    const float rows = static_cast<float>(view.groups().size() + 1) + (f.scrolls ? page_rows : 0);
    const float controls_scale = std::min({1.0f, f.strip / spec_view_controls_width,
        f.height / (16 + rows * (theme.line_height + 16) + (rows - 1) * 6)});
    controls.width = f.strip;
    controls.padding = 8 * controls_scale;
    controls.gap = 6 * controls_scale;
    controls.positioned = true;
    const Layout row{.padding = 8 * controls_scale, .text_scale = controls_scale};
    document.panel("spec-view/controls", [&] {
        const auto& info = view.info();
        document.text("spec-view/controls/title",
            (info.project.empty() ? std::string("Specifications") : info.project)
                + (info.version.empty() ? std::string() : " " + info.version), row);
        for(const auto& group : view.groups()) {
            const std::string id = group.id;
            const std::string name = group.name.empty() ? id : group.name;
            document.button("spec-view/toggle/" + id, (group.visible ? "ON  " : "OFF ") + name,
                [toggle, id] { if(toggle) toggle(id); }, row);
        }
        if(f.scrolls) declare_page_controls(document, page_position(view, f), navigate, row);
    }, controls);
}

// A paged canvas covers the shown rows only, so it never reaches past the host.
Rect card_canvas(const Frame& f) {
    return f.scrolls ? Rect{f.area.x, 0, f.area.width, f.height} : f.area;
}

void declare_group(Document& document, const SpecView& view, const SpecViewGroup& group, Color color, const Frame& f, const Theme& theme) {
    const Rect& area = f.area;
    const float scale = f.scale;
    std::vector<const SpecViewCard*> members;
    for(const auto& card : view.cards())
        if(card.group_id == group.id && card_shown(card.bounds, f)) members.push_back(&card);
    const Rect canvas = card_canvas(f);
    const float shift = canvas.y - area.y;
    const Layout canvas_layout{.width = layout_extent(canvas.width), .height = layout_extent(canvas.height),
        .padding = 0, .positioned = true, .x = canvas.x, .y = canvas.y};
    for(std::size_t start = 0; start < members.size(); start += shapes_per_canvas) {
        Drawing drawing;
        const auto end = std::min(members.size(), start + shapes_per_canvas);
        for(std::size_t index = start; index < end; ++index) {
            const auto& b = members[index]->bounds;
            drawing.rectangle({b.x * scale, b.y * scale - shift, b.width * scale, b.height * scale},
                tint(color), {color, outline_width}, corner_radius);
        }
        document.canvas(group_prefix(group.id) + "/shapes/" + std::to_string(start / shapes_per_canvas),
            std::move(drawing), canvas_layout);
    }
    for(const auto* card : members) {
        const auto& b = card->bounds;
        const float width = (b.width - card_padding * 2) * scale;
        if(width <= 0 || b.height <= code_offset) continue;
        const float x = area.x + (b.x + card_padding) * scale;
        document.text(group_prefix(group.id) + "/code/" + card->code, card_heading(*card),
            {.width = width, .height = std::min(theme.line_height, b.height - code_offset) * scale,
             .padding = 0, .positioned = true, .x = x, .y = area.y + (b.y + code_offset) * scale, .text_scale = scale});
        const float title_start = std::max(title_offset, code_offset + theme.line_height);
        const float title_room = b.height - title_start - card_padding;
        if(title_room < theme.line_height) continue;
        const unsigned title_lines = static_cast<unsigned>(std::min(3.0f, title_room / theme.line_height));
        document.text(group_prefix(group.id) + "/title/" + card->code, card->title,
            {.width = width, .height = title_lines * theme.line_height * scale, .padding = 0,
             .positioned = true, .x = x, .y = area.y + (b.y + title_start) * scale,
             .lines = title_lines, .text_scale = scale});
    }
}
}

Rect spec_view_area(const SpecViewInfo& info, Rect available) {
    if(available.width <= 0 || available.height <= 0 || info.width <= 0 || info.height <= 0)
        return {available.x, available.y, 0, 0};
    const float fitted = std::min(available.width / info.width, available.height / info.height);
    // Shrinking a tall view below scale 1 makes its text unreadable; fit its width and page instead.
    const float scale = fitted >= 1 ? std::min(fitted, 64.0f) : std::min(available.width / info.width, 1.0f);
    const float width = info.width * scale, height = info.height * scale;
    const float y = height > available.height ? available.y : available.y + (available.height - height) / 2;
    return {available.x + (available.width - width) / 2, y, width, height};
}

float spec_view_page(const SpecViewInfo& info, const Viewport& viewport) {
    const auto f = view_frame(info, viewport, 0);
    return f ? f->page : info.height;
}

Document spec_view_document(const SpecView& view, const Viewport& viewport,
    std::function<void(const std::string& group_id)> toggle, const Theme& theme,
    std::function<void(SpecViewAction)> navigate) {
    if(!std::isfinite(theme.line_height) || theme.line_height <= 0)
        throw std::invalid_argument("Spec view requires a positive finite line height");
    Document document;
    const auto f = view_frame(view.info(), viewport, view.scroll());
    if(!f) return document;
    for(std::size_t index = 0; index < view.groups().size(); ++index) {
        const auto& group = view.groups()[index];
        if(group.visible) declare_group(document, view, group, group_palette[index % group_palette.size()], *f, theme);
    }
    declare_controls(document, view, *f, theme, toggle, navigate);
    return document;
}
}
