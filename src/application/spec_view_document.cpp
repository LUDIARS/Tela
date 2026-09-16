// @spec SPEC-TL-SPEC-VIEW
#include <tela/spec_view.hpp>
#include <algorithm>
#include <array>

namespace tela {
namespace {
constexpr std::size_t shapes_per_canvas = 200; // Drawing accepts at most 256 commands
constexpr float max_layout_extent = 32768;     // Document layout dimension limit
constexpr float controls_width = 280, controls_margin = 12;
constexpr float card_padding = 6, outline_width = 1.5f, corner_radius = 6;
constexpr float code_offset = 4, title_offset = 26;
// Pf sizes the cards for the default 16 px theme, whose line is 24 px. The title takes the
// rows left under the code line, so a long title wraps instead of losing everything past
// the first break.
constexpr float assumed_line_height = 24;
// Pf draws the cards on a dark canvas (#1a1e2a) tinted 19% toward the group color. The overlay
// has no canvas of its own, so it pre-blends the same result and the near-white text stays
// readable over a light host as well as a dark one.
constexpr Color canvas_base{26, 31, 42, 235};
constexpr float accent_mix = 0.19f;
constexpr unsigned char mix_channel(unsigned char base, unsigned char accent) {
    return static_cast<unsigned char>(base + (accent - base) * accent_mix + 0.5f);
}
constexpr Color card_fill(Color accent) {
    return {mix_channel(canvas_base.r, accent.r), mix_channel(canvas_base.g, accent.g),
            mix_channel(canvas_base.b, accent.b), canvas_base.a};
}
// Groups are told apart by outline color; the palette repeats beyond six groups.
// Kept in the same order as Pf's SPEC_VIEW_PALETTE so both views draw the same picture.
constexpr std::array<Color, 6> palette{{
    {120, 200, 255, 255}, {255, 196, 90, 255}, {150, 230, 150, 255},
    {255, 140, 170, 255}, {200, 160, 255, 255}, {240, 240, 120, 255}}};

std::string group_prefix(const std::string& group_id) { return "spec-view/group/" + group_id; }
float layout_extent(float value) { return std::clamp(value, 1.0f, max_layout_extent); }

std::string card_heading(const SpecViewCard& card) {
    std::string heading = card.code;
    if(!card.status.empty()) heading += " " + card.status;
    return heading + " v" + std::to_string(card.version);
}

void declare_group(Document& document, const SpecView& view, const SpecViewGroup& group, Color color, Rect area) {
    const float scale = area.width / view.info().width;
    std::vector<const SpecViewCard*> members;
    for(const auto& card : view.cards())
        if(card.group_id == group.id) members.push_back(&card);
    const Layout canvas_layout{.width = layout_extent(area.width), .height = layout_extent(area.height),
        .padding = 0, .positioned = true, .x = area.x, .y = area.y};
    for(std::size_t start = 0; start < members.size(); start += shapes_per_canvas) {
        Drawing drawing;
        const auto end = std::min(members.size(), start + shapes_per_canvas);
        for(std::size_t index = start; index < end; ++index) {
            const auto& b = members[index]->bounds;
            drawing.rectangle({b.x * scale, b.y * scale, b.width * scale, b.height * scale},
                card_fill(color), {color, outline_width}, corner_radius);
        }
        document.canvas(group_prefix(group.id) + "/shapes/" + std::to_string(start / shapes_per_canvas),
            std::move(drawing), canvas_layout);
    }
    for(const auto* card : members) {
        const auto& b = card->bounds;
        const float width = layout_extent(b.width * scale - card_padding * 2);
        const float x = area.x + b.x * scale + card_padding;
        document.text(group_prefix(group.id) + "/code/" + card->code, card_heading(*card),
            {.width = width, .padding = 0, .positioned = true, .x = x, .y = area.y + b.y * scale + code_offset});
        const float title_room = b.height * scale - title_offset - card_padding;
        const unsigned title_lines = std::max(1u, static_cast<unsigned>(title_room / assumed_line_height));
        document.text(group_prefix(group.id) + "/title/" + card->code, card->title,
            {.width = width, .padding = 0, .positioned = true, .x = x, .y = area.y + b.y * scale + title_offset, .lines = title_lines});
    }
}
}

Rect spec_view_area(const SpecViewInfo& info, Rect available) {
    if(available.width <= 0 || available.height <= 0 || info.width <= 0 || info.height <= 0)
        return {available.x, available.y, 0, 0};
    const float scale = std::min(available.width / info.width, available.height / info.height);
    const float width = info.width * scale, height = info.height * scale;
    return {available.x + (available.width - width) / 2, available.y + (available.height - height) / 2, width, height};
}

Document spec_view_document(const SpecView& view, const Viewport& viewport,
    std::function<void(const std::string& group_id)> toggle) {
    Document document;
    // A hidden or degenerate host declares nothing, matching the overlay lifecycle contract.
    if(!viewport.visible || viewport.width <= 0 || viewport.height <= 0 || !(viewport.dpi_scale > 0)) return document;
    const Rect area = spec_view_area(view.info(),
        {0, 0, viewport.width / viewport.dpi_scale, viewport.height / viewport.dpi_scale});
    if(area.width <= 0 || area.height <= 0) return document;
    for(std::size_t index = 0; index < view.groups().size(); ++index) {
        const auto& group = view.groups()[index];
        if(group.visible) declare_group(document, view, group, palette[index % palette.size()], area);
    }
    Layout controls;
    controls.width = controls_width;
    controls.positioned = true;
    controls.x = controls_margin;
    controls.y = controls_margin;
    // Declared last so the toggles stay above the drawn cards.
    document.panel("spec-view/controls", [&] {
        const auto& info = view.info();
        document.text("spec-view/controls/title",
            (info.project.empty() ? std::string("Specifications") : info.project)
                + (info.version.empty() ? std::string() : " " + info.version));
        for(const auto& group : view.groups()) {
            const std::string id = group.id;
            const std::string name = group.name.empty() ? id : group.name;
            document.button("spec-view/toggle/" + id, (group.visible ? "ON  " : "OFF ") + name, [toggle, id] { toggle(id); });
        }
    }, controls);
    return document;
}
}
