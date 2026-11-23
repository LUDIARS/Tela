// @spec SPEC-TL-INPUT
#include <tela/runtime.hpp>
#include <algorithm>

// @implements SPEC-TL-CONTROLS
// @spec Controls
namespace tela {
namespace {
bool actionable(ElementKind kind) { return kind == ElementKind::button || kind == ElementKind::list_item; }
bool text_kind(ElementKind kind) { return kind == ElementKind::text_field || kind == ElementKind::text_area; }
constexpr float wheel_lines = 3; // rows scrolled per wheel notch
}
bool Runtime::pointer(const HostPointerEvent& event, InputSource source) {
    const float x = (event.desktop_x-viewport_.desktop_x)/viewport_.dpi_scale;
    const float y = (event.desktop_y-viewport_.desktop_y)/viewport_.dpi_scale;
    auto e = target(x,y);
    const auto decision=ownership_.apply(event,source,viewport_.revision,viewport_.visible&&viewport_.focused,
        e?InputTarget{e->element.id,e->element.input,actionable(e->element.kind)}:InputTarget{});
    if(event.phase==PointerPhase::wheel)return decision.accepted&&wheel(event,{x,y});
    if(!decision.pressed.empty()){states_.at(decision.pressed).pressed=true;dirty_=true;}
    if(!decision.released.empty()){states_.at(decision.released).pressed=false;dirty_=true;}
    // A press anywhere that is not a control moves focus away from the current one.
    if(decision.accepted&&event.phase==PointerPhase::down&&event.button==PointerButton::primary){
        const auto* owner=decision.pressed.empty()?nullptr:declared(decision.pressed);
        if(!owner||!focusable(*owner))blur();
    }
    // The gesture owner, not whatever lies under the pointer, follows a drag.
    if(decision.accepted&&(event.phase==PointerPhase::down||event.phase==PointerPhase::move))
        if(const auto gesture=ownership_.active();gesture&&gesture->source==source)
            track(gesture->id,{x,y},event.phase==PointerPhase::down,event.modifiers);
    // Dispatch the owner's action, not whatever sits under the release point.
    if(!decision.activated.empty()){
        const auto* owner=e&&e->element.id==decision.activated?e:placed(decision.activated);
        ++states_.at(decision.activated).activations;
        // Copy before invoking: an action may redeclare the document, which
        // reassigns placed_ and would otherwise free the std::function mid-call.
        if(owner&&owner->element.action){auto action=owner->element.action;action();}
    }
    return decision.consumed;
}
void Runtime::track(const std::string& id, Point point, bool start, std::uint32_t modifiers) {
    const auto* p = placed(id);
    if (!p) return;
    const auto& e = p->element;
    if (e.kind == ElementKind::slider) {
        if (start) focus(id);
        set_fraction(e, slider_fraction_at(e, p->bounds, point.x));
        return;
    }
    if (!text_kind(e.kind)) return;
    auto& s = states_[id];
    const auto caret = field_caret_at(e, p->bounds, {s.caret, s.anchor}, point, theme_, measure_);
    if (start) {
        const bool extend = (modifiers & modifier_shift) && focused_ == id;
        if (focused_ != id) { focused_ = id; reveal(id); }
        auto& current = states_[id];
        current.caret = caret;
        if (!extend) current.anchor = caret;
    } else {
        s.caret = caret;
    }
    dirty_ = true;
}
void Runtime::set_fraction(const Element& e, float fraction) {
    if (fraction == e.value.fraction || !e.on_set) return;
    auto set = e.on_set; // the callback may redeclare and free `e`
    set(fraction);
}
bool Runtime::wheel(const HostPointerEvent& event, Point point) {
    const float delta = -event.wheel_y * wheel_lines * theme_.line_height;
    bool over = false;
    // Innermost container first; a container already at its limit passes the wheel outward.
    for (auto it = placed_.rbegin(); it != placed_.rend(); ++it) {
        if (it->element.kind != ElementKind::scroll || !it->clip.contains(point.x, point.y)) continue;
        over = true;
        const float visible = std::max(0.f, it->bounds.height - 2 * it->element.layout.padding);
        const float wanted = std::clamp(it->scroll + delta, 0.f, std::max(0.f, it->content_height - visible));
        if (wanted == it->scroll) continue;
        states_[it->element.id].scroll = wanted;
        layout();
        return true;
    }
    return over;
}
}
