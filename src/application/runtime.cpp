// @spec SPEC-TL-RUNTIME
#include <tela/runtime.hpp>
#include <tela/utf8.hpp>
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace tela {
void Runtime::layout() {
    const auto scale = viewport_.dpi_scale;
    placed_ = arrange(document_, viewport_.width/scale, viewport_.height/scale, theme_);
    dirty_ = true;
}
void Runtime::document(Document document) {
    const auto& before=document_.elements();const auto& after=document.elements();
    const bool same=before.size()==after.size()&&std::equal(before.begin(),before.end(),after.begin(),[](const auto& a,const auto& b){
        return a.id==b.id&&a.parent==b.parent&&a.kind==b.kind&&a.label==b.label&&a.input==b.input&&a.layout==b.layout&&a.drawing==b.drawing;
    });
    if(same){
        document_=std::move(document);
        // Rebind callbacks without submitting an identical visual frame. Bind by
        // stable ID: arrange() emits placed_ in tree order, which is not required
        // to match declaration order.
        std::unordered_map<std::string,const std::function<void()>*> actions;
        for(const auto& e:document_.elements())actions.emplace(e.id,&e.action);
        for(auto& p:placed_){
            auto it=actions.find(p.element.id);
            p.element.action=it==actions.end()?std::function<void()>{}:*it->second;
        }
        return;
    }
    std::unordered_map<std::string, ElementState> next;
    for (const auto& e : document.elements()) {
        auto old = states_.find(e.id);
        next[e.id] = old == states_.end() ? ElementState{} : old->second;
    }
    // A removed or retyped action cannot inherit an in-flight click.
    if (const auto gesture = ownership_.active()) {
        bool retained = false;
        for (const auto& e : document.elements())
            if (e.id == gesture->id && e.kind == ElementKind::button &&
                ((gesture->source == InputSource::native && e.input == InputPolicy::exclusive) ||
                 (gesture->source == InputSource::host_observation && e.input == InputPolicy::shared))) retained = true;
        if (!retained) { cancel(); for (auto& [id,s] : next) s.pressed = false; }
    }
    states_ = std::move(next);
    document_ = std::move(document);
    layout();
    // Focus follows the stable ID; a field that disappeared or changed kind loses it.
    if (!focused_.empty()) {
        const auto* p = placed(focused_);
        if (!p || p->element.kind != ElementKind::field) { focused_.clear(); focus_original_.clear(); }
    }
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
    cancel(); viewport_.visible = false; ownership_.reset();
    viewport_.revision = 0;
}
const ElementState* Runtime::state(const std::string& id) const {
    auto it = states_.find(id); return it == states_.end() ? nullptr : &it->second;
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
bool Runtime::pointer(const HostPointerEvent& event, InputSource source) {
    const float x = (event.desktop_x-viewport_.desktop_x)/viewport_.dpi_scale;
    const float y = (event.desktop_y-viewport_.desktop_y)/viewport_.dpi_scale;
    auto e = target(x,y);
    // Pressing anywhere but the focused field ends its editing.
    if(event.phase==PointerPhase::down&&!focused_.empty()&&(!e||e->element.id!=focused_))blur();
    const bool actionable=e&&(e->element.kind==ElementKind::button||e->element.kind==ElementKind::field);
    const auto decision=ownership_.apply(event,source,viewport_.revision,viewport_.visible&&viewport_.focused,
        e?InputTarget{e->element.id,e->element.input,actionable}:InputTarget{});
    if(!decision.pressed.empty()){states_.at(decision.pressed).pressed=true;dirty_=true;}
    if(!decision.released.empty()){states_.at(decision.released).pressed=false;dirty_=true;}
    // Dispatch the owner's action, not whatever sits under the release point.
    if(!decision.activated.empty()){
        const auto* owner=e&&e->element.id==decision.activated?e:nullptr;
        if(!owner)for(const auto& p:placed_)if(p.element.id==decision.activated){owner=&p;break;}
        ++states_.at(decision.activated).activations;
        if(owner&&owner->element.kind==ElementKind::field){focus(owner->element.id);return decision.consumed;}
        // Copy before invoking: an action may redeclare the document, which
        // reassigns placed_ and would otherwise free the std::function mid-call.
        if(owner&&owner->element.action){auto action=owner->element.action;action();}
    }
    return decision.consumed;
}
const PlacedElement* Runtime::placed(const std::string& id) const {
    for (const auto& p : placed_) if (p.element.id == id) return &p;
    return nullptr;
}
void Runtime::focus(const std::string& id) {
    const auto* p = placed(id);
    if (!p || p->element.kind != ElementKind::field) throw std::invalid_argument("Tela can only focus a field");
    if (focused_ == id) return;
    focused_ = id;
    focus_original_ = p->element.label;
    dirty_ = true;
}
void Runtime::blur() {
    if (focused_.empty()) return;
    focused_.clear();
    focus_original_.clear();
    dirty_ = true;
}
void Runtime::edit_focused(const std::string& value) {
    const auto* p = placed(focused_);
    if (!p || !p->element.edit) return;
    // Copy first: the callback usually redeclares the document, which replaces placed_.
    auto edit = p->element.edit;
    dirty_ = true;
    edit(value);
}
bool Runtime::text(const std::string& utf8) {
    if (focused_.empty()) return false;
    const auto* p = placed(focused_);
    if (!p) return false;
    std::string value = p->element.label;
    for (std::size_t at = 0; at < utf8.size();) {
        const auto bytes = utf8_character_bytes(utf8, at);
        if (bytes == 0) { ++at; continue; }                       // not UTF-8: skip the byte
        const auto lead = static_cast<unsigned char>(utf8[at]);
        const bool control = bytes == 1 && (lead < 0x20 || lead == 0x7F);
        if (!control && value.size() + bytes <= 16384) value.append(utf8, at, bytes);
        at += bytes;
    }
    if (value != p->element.label) edit_focused(value);
    return true;
}
bool Runtime::key(EditKey key) {
    if (focused_.empty()) return false;
    const auto* p = placed(focused_);
    if (!p) { blur(); return false; }
    switch (key) {
    case EditKey::backspace: {
        std::string value = p->element.label;
        if (value.empty()) return true;
        std::size_t last = 0;
        for (std::size_t at = 0; at < value.size();) {
            const auto bytes = std::max<std::size_t>(1, utf8_character_bytes(value, at));
            last = at;
            at += bytes;
        }
        value.erase(last);
        edit_focused(value);
        return true;
    }
    case EditKey::enter:
        blur();
        return true;
    case EditKey::escape: {
        const std::string original = focus_original_;
        if (p->element.label != original) edit_focused(original);
        blur();
        return true;
    }
    case EditKey::tab:
    case EditKey::back_tab: {
        std::vector<std::string> fields;
        for (const auto& placed_element : placed_)
            if (placed_element.element.kind == ElementKind::field) fields.push_back(placed_element.element.id);
        const auto here = std::find(fields.begin(), fields.end(), focused_);
        const auto index = static_cast<std::size_t>(here - fields.begin());
        const auto count = fields.size();
        const auto next = key == EditKey::tab ? (index + 1) % count : (index + count - 1) % count;
        blur();
        focus(fields[next]);
        return true;
    }
    }
    return true;
}
}
