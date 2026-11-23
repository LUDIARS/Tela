// @spec SPEC-TL-CONTROLS
#include <tela/control_visual.hpp>
#include <tela/runtime.hpp>
#include <algorithm>

namespace tela {
namespace {
Color alpha(Color c, unsigned char a) { c.a = a; return c; }
Color tone(const Theme& t, Tone tone) {
    switch (tone) {
    case Tone::good: return t.good;
    case Tone::warning: return t.warning;
    case Tone::bad: return t.bad;
    default: return t.muted;
    }
}
class Look {
public:
    explicit Look(const ControlLook& look) : l(look), e(look.placed.element), b(look.placed.bounds) {
        const float p = e.layout.padding;
        area = {b.x+p, b.y+p, std::max(0.f,b.width-2*p), std::max(0.f,b.height-2*p)};
        font = l.theme.font_size*e.layout.text_scale;
        line = l.theme.line_height*e.layout.text_scale;
    }
    std::vector<VisualItem> run() {
        switch (e.kind) {
        case ElementKind::text_field: case ElementKind::text_area: field(); break;
        case ElementKind::slider: slider(); break;
        case ElementKind::list_item: list_item(); break;
        case ElementKind::separator: rect({b.x, b.y+b.height/2, b.width, 1}, alpha(l.theme.muted, 140)); break;
        case ElementKind::status: status(); break;
        case ElementKind::scroll: scroll_bar(); break;
        case ElementKind::video: video(); break;
        default: break;
        }
        return std::move(items);
    }
private:
    const ControlLook& l;
    const Element& e;
    Rect b, area;
    float font{}, line{};
    std::vector<VisualItem> items;

    void rect(Rect r, Color c, float border = 0) { items.push_back(VisualRect{r, c, border}); }
    void text(std::string value, Point at, Color c) {
        if (!value.empty()) items.push_back(VisualText{std::move(value), at, c, area, font, line});
    }
    bool pressed() const { return l.state && l.state->pressed; }
    void field() {
        const auto& t = l.theme;
        rect(b, t.field);
        if (l.focused) rect(b, t.accent, 1);
        const text_edit::Selection selection = l.state ? text_edit::Selection{l.state->caret, l.state->anchor} : text_edit::Selection{};
        const auto view = field_view(e, b, selection, l.focused, t, l.measure);
        for (const auto& r : view.selection) rect(intersect(r, area), alpha(t.accent, 110));
        const auto color = view.placeholder ? t.muted : t.text;
        for (const auto& row : view.lines)
            text(view.display.substr(row.begin, row.end-row.begin), row.origin, color);
        if (view.caret.width > 0) rect(view.caret, t.text);
    }
    void slider() {
        const auto& t = l.theme;
        const float track = std::max(2.f, line/6);
        const float y = area.y + (area.height-track)/2;
        const float knob_x = area.x + area.width*e.value.fraction;
        rect({area.x, y, area.width, track}, alpha(t.muted, 120));
        rect({area.x, y, knob_x-area.x, track}, t.accent);
        const float knob = std::max(6.f, line/2.5f), height = std::min(area.height, line*.8f);
        rect({knob_x-knob/2, area.y+(area.height-height)/2, knob, height}, l.focused || pressed() ? t.accent : t.text);
    }
    void list_item() {
        const auto& t = l.theme;
        if (pressed()) rect(b, t.pressed);
        else if (e.value.selected) rect(b, t.button);
        if (e.value.selected) rect({b.x, b.y, 3, b.height}, t.accent);
        text(e.label, {area.x, area.y}, t.text);
        if (e.value.text.empty()) return;
        const float width = l.measure(e.value.text, font);
        text(e.value.text, {std::max(area.x, area.x+area.width-width), area.y}, t.muted); // right-aligned detail
    }
    void status() {
        const float dot = font*.6f;
        items.push_back(VisualEllipse{{area.x, area.y+(line-dot)/2, dot, dot}, tone(l.theme, e.value.tone)});
        text(e.label, {area.x+line, area.y}, l.theme.text);
    }
    void scroll_bar() {
        const float total = l.placed.content_height;
        if (total <= area.height || area.height <= 0) return;
        const float thumb = std::max(line, area.height*area.height/total);
        const float y = area.y + (area.height-thumb)*(l.placed.scroll/(total-area.height));
        rect({b.x+b.width-5, y, 4, thumb}, alpha(l.theme.muted, 160));
    }
    void video() {
        rect(b, {0, 0, 0, 200});
        rect(b, l.theme.muted, 1);
        text(e.label, {area.x, area.y}, l.theme.text);
    }
};
}
bool is_control(ElementKind kind) noexcept {
    switch (kind) {
    case ElementKind::panel: case ElementKind::text: case ElementKind::button: case ElementKind::canvas: return false;
    default: return true;
    }
}
std::vector<VisualItem> control_visual(const ControlLook& look) { return Look(look).run(); }
}
