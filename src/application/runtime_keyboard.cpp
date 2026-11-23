// @spec SPEC-TL-CONTROLS
#include <tela/runtime.hpp>
#include <algorithm>

// @implements SPEC-TL-CONTROLS
// @spec Controls
namespace tela {
namespace {
constexpr std::size_t text_event_limit = 4096;
constexpr std::size_t value_limit = 16384;
constexpr float slider_step = .05f, slider_page = .25f;
bool text_kind(ElementKind kind) { return kind == ElementKind::text_field || kind == ElementKind::text_area; }
}
bool Runtime::focus(const std::string& id) {
    const auto* e = declared(id);
    if (!e || !focusable(*e)) return false;
    if (focused_ == id) return true;
    focused_ = id;
    auto& s = states_[id];
    // Arriving by Tab or by call puts the caret after the value, as a field usually does.
    if (text_kind(e->kind)) s.caret = s.anchor = e->value.text.size();
    dirty_ = true;
    reveal(id);
    return true;
}
void Runtime::blur() {
    if (focused_.empty()) return;
    focused_.clear();
    dirty_ = true;
}
bool Runtime::step_focus(int direction) {
    std::vector<std::string> order;
    for (const auto& p : placed_) if (focusable(p.element)) order.push_back(p.element.id);
    if (order.empty()) return false;
    const auto at = std::find(order.begin(), order.end(), focused_);
    std::size_t index = 0;
    if (at == order.end()) index = direction > 0 ? 0 : order.size() - 1;
    else index = (static_cast<std::size_t>(at - order.begin()) + order.size() + direction) % order.size();
    return focus(order[index]);
}
void Runtime::reveal(const std::string& id) {
    const auto* p = placed(id);
    if (!p) return;
    const auto bounds = p->bounds;
    // Nearest enclosing scroll container.
    for (auto parent = p->element.parent; !parent.empty();) {
        const auto* container = placed(parent);
        if (!container) return;
        if (container->element.kind == ElementKind::scroll) {
            const float pad = container->element.layout.padding;
            const float top = container->bounds.y + pad;
            const float bottom = container->bounds.y + container->bounds.height - pad;
            float offset = container->scroll;
            if (bounds.y < top) offset -= top - bounds.y;
            else if (bounds.y + bounds.height > bottom) offset += std::min(bounds.y - top, bounds.y + bounds.height - bottom);
            if (offset != container->scroll) { states_[parent].scroll = std::max(0.f, offset); layout(); }
            return;
        }
        parent = container->element.parent;
    }
}
bool Runtime::key(const HostKeyEvent& event) {
    if (event.sequence <= key_sequence_) return false;
    key_sequence_ = event.sequence;
    if (!viewport_.visible || !viewport_.focused || !event.pressed) return false;
    if (event.key == Key::tab) return step_focus(event.modifiers & modifier_shift ? -1 : 1);
    const auto* e = declared(focused_);
    if (!e) return false;
    if (event.key == Key::escape) { blur(); return true; }
    if (e->kind == ElementKind::slider) return slider_key(*e, event.key);
    return edit_key(*e, event);
}
bool Runtime::slider_key(const Element& e, Key key) {
    float fraction = e.value.fraction;
    switch (key) {
    case Key::left: case Key::down: fraction -= slider_step; break;
    case Key::right: case Key::up: fraction += slider_step; break;
    case Key::page_down: fraction -= slider_page; break;
    case Key::page_up: fraction += slider_page; break;
    case Key::home: fraction = 0; break;
    case Key::end: fraction = 1; break;
    default: return false;
    }
    set_fraction(e, std::clamp(fraction, 0.f, 1.f));
    return true;
}
bool Runtime::edit_key(const Element& e, const HostKeyEvent& event) {
    auto& s = states_[e.id];
    const std::string_view value = e.value.text;
    const text_edit::Selection selection{text_edit::clamp(value, s.caret), text_edit::clamp(value, s.anchor)};
    const bool shift = event.modifiers & modifier_shift;
    const bool command = event.modifiers & (modifier_control | modifier_meta);
    const bool area = e.kind == ElementKind::text_area;
    const auto rows = static_cast<int>(std::max(1u, e.layout.lines));
    std::size_t caret = selection.caret;
    switch (event.key) {
    case Key::left:
        caret = !shift && !selection.empty() ? selection.start() : text_edit::previous(value, caret); break;
    case Key::right:
        caret = !shift && !selection.empty() ? selection.end() : text_edit::next(value, caret); break;
    case Key::home: caret = command ? 0 : text_edit::line_start(value, caret); break;
    case Key::end: caret = command ? value.size() : text_edit::line_end(value, caret); break;
    case Key::up: if (!area) return false; caret = text_edit::vertical(value, caret, -1); break;
    case Key::down: if (!area) return false; caret = text_edit::vertical(value, caret, 1); break;
    case Key::page_up: if (!area) return false; caret = text_edit::vertical(value, caret, -rows); break;
    case Key::page_down: if (!area) return false; caret = text_edit::vertical(value, caret, rows); break;
    case Key::a:
        if (!command) return false;
        s.anchor = 0; s.caret = value.size(); dirty_ = true; return true;
    case Key::backspace: change(e, text_edit::erase(value, selection, false)); return true;
    case Key::delete_forward: change(e, text_edit::erase(value, selection, true)); return true;
    case Key::enter:
        if (area) { change(e, text_edit::replace(value, selection, "\n", value_limit)); return true; }
        if (e.action) { auto submit = e.action; submit(); }
        return true;
    default: return false;
    }
    s.caret = caret;
    if (!shift) s.anchor = caret;
    dirty_ = true;
    return true;
}
bool Runtime::text(const HostTextEvent& event) {
    if (event.sequence <= key_sequence_) return false;
    key_sequence_ = event.sequence;
    if (!viewport_.visible || !viewport_.focused || event.text.size() > text_event_limit) return false;
    const auto* e = declared(focused_);
    if (!e || !text_kind(e->kind)) return false;
    const auto insert = text_edit::sanitize(event.text, e->kind == ElementKind::text_area);
    if (insert.empty()) return false;
    const auto& s = states_[e->id];
    change(*e, text_edit::replace(e->value.text, {s.caret, s.anchor}, insert, value_limit));
    return true;
}
void Runtime::change(const Element& e, text_edit::Edit edit) {
    auto& s = states_[e.id];
    s.caret = s.anchor = edit.caret;
    dirty_ = true;
    if (edit.value == e.value.text || !e.on_change) return;
    auto notify = e.on_change; // the application redeclares from inside, which frees `e`
    notify(edit.value);
}
}
