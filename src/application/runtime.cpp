// @spec SPEC-TL-RUNTIME
#include <tela/runtime.hpp>
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace tela {
namespace {
bool same_visual(const Element& a, const Element& b) {
    return a.id==b.id&&a.parent==b.parent&&a.kind==b.kind&&a.label==b.label&&a.input==b.input&&
        a.layout==b.layout&&a.drawing==b.drawing&&a.value==b.value;
}
bool text_kind(ElementKind kind) { return kind == ElementKind::text_field || kind == ElementKind::text_area; }
}
void Runtime::layout() {
    const auto scale = viewport_.dpi_scale;
    ScrollOffsets offsets;
    for (const auto& [id, s] : states_) if (s.scroll > 0) offsets.emplace(id, s.scroll);
    placed_ = arrange(document_, viewport_.width/scale, viewport_.height/scale, theme_, offsets);
    // Keep the clamped offset, so content that shrank does not leave a stale position behind.
    for (const auto& p : placed_)
        if (p.element.kind == ElementKind::scroll) states_[p.element.id].scroll = p.scroll;
    dirty_ = true;
}
void Runtime::document(Document document) {
    const auto& before=document_.elements();const auto& after=document.elements();
    const bool same=before.size()==after.size()&&std::equal(before.begin(),before.end(),after.begin(),same_visual);
    if(same){
        document_=std::move(document);
        // Rebind callbacks without submitting an identical visual frame. Bind by
        // stable ID: arrange() emits placed_ in tree order, which is not required
        // to match declaration order.
        std::unordered_map<std::string,const Element*> latest;
        for(const auto& e:document_.elements())latest.emplace(e.id,&e);
        for(auto& p:placed_){
            auto it=latest.find(p.element.id);
            if(it==latest.end()){p.element.action={};p.element.on_change={};p.element.on_set={};continue;}
            p.element.action=it->second->action;
            p.element.on_change=it->second->on_change;
            p.element.on_set=it->second->on_set;
        }
        return;
    }
    std::unordered_map<std::string, ElementState> next;
    for (const auto& e : document.elements()) {
        auto old = states_.find(e.id);
        auto state = old == states_.end() ? ElementState{} : old->second;
        // The application may have rejected or rewritten the edit; stay inside its value.
        if (text_kind(e.kind)) {
            state.caret = text_edit::clamp(e.value.text, state.caret);
            state.anchor = text_edit::clamp(e.value.text, state.anchor);
        }
        next[e.id] = state;
    }
    // A removed or retyped control cannot inherit an in-flight gesture.
    if (const auto gesture = ownership_.active()) {
        const auto* previous = declared(gesture->id);
        bool retained = false;
        for (const auto& e : document.elements())
            if (e.id == gesture->id && previous && e.kind == previous->kind &&
                ((gesture->source == InputSource::native && e.input == InputPolicy::exclusive) ||
                 (gesture->source == InputSource::host_observation && e.input == InputPolicy::shared))) retained = true;
        if (!retained) { cancel(); for (auto& [id,s] : next) s.pressed = false; }
    }
    states_ = std::move(next);
    document_ = std::move(document);
    if (!focused_.empty()) {
        const auto* e = declared(focused_);
        if (!e || !focusable(*e)) focused_.clear();
    }
    layout();
}
void Runtime::theme(Theme value) {
    if (!std::isfinite(value.font_size) || !std::isfinite(value.line_height) ||
        value.font_size <= 0 || value.font_size > 256 || value.line_height <= 0 || value.line_height > 512)
        throw std::invalid_argument("Invalid Tela theme metrics");
    if (value == theme_) return;
    theme_ = value; layout();
}
void Runtime::viewport(Viewport value) {
    if (!std::isfinite(value.dpi_scale) || value.dpi_scale < .25f || value.dpi_scale > 8 ||
        value.width < 0 || value.height < 0 || value.width > 16384 || value.height > 16384 ||
        static_cast<std::uint64_t>(value.width)*value.height > 16777216)
        throw std::invalid_argument("Invalid Tela viewport");
    const bool same = value.host_id == viewport_.host_id && value.view_id == viewport_.view_id;
    if (same && value.revision < viewport_.revision) return;
    const bool geometry = !same || value.width != viewport_.width || value.height != viewport_.height ||
        value.dpi_scale != viewport_.dpi_scale || value.desktop_x != viewport_.desktop_x || value.desktop_y != viewport_.desktop_y;
    if (!same || !value.visible || (!value.focused && viewport_.focused)) cancel();
    const bool shown = value.visible && !viewport_.visible;
    viewport_ = std::move(value);
    if (geometry || shown) layout();
}
void Runtime::disconnect() {
    cancel(); blur(); viewport_.visible = false; ownership_.reset();
    viewport_.revision = 0; key_sequence_ = 0;
}
void Runtime::text_measure(TextMeasure measure) {
    if (!measure) throw std::invalid_argument("Tela text measure is required");
    measure_ = std::move(measure);
    dirty_ = true;
}
const ElementState* Runtime::state(const std::string& id) const {
    auto it = states_.find(id); return it == states_.end() ? nullptr : &it->second;
}
const PlacedElement* Runtime::placed(const std::string& id) const {
    for (const auto& p : placed_) if (p.element.id == id) return &p;
    return nullptr;
}
const Element* Runtime::declared(const std::string& id) const {
    for (const auto& e : document_.elements()) if (e.id == id) return &e;
    return nullptr;
}
bool Runtime::focusable(const Element& e) noexcept {
    return e.input != InputPolicy::passthrough && (text_kind(e.kind) || e.kind == ElementKind::slider);
}
const PlacedElement* Runtime::target(float x, float y) const {
    for (auto it = placed_.rbegin(); it != placed_.rend(); ++it)
        if (it->element.input != InputPolicy::passthrough && it->clip.contains(x,y)) return &*it;
    return nullptr;
}
InputPolicy Runtime::hit(float x, float y) const {
    auto e = target(x,y); return e ? e->element.input : InputPolicy::passthrough;
}
void Runtime::cancel() {
    const auto id=ownership_.cancel();if(id.empty())return;
    auto it = states_.find(id); if (it != states_.end()) it->second.pressed = false;
    dirty_ = true;
}
}
