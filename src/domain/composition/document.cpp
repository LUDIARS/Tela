// @spec SPEC-TL-DECLARATION
#include <tela/document.hpp>
#include <algorithm>
#include <stdexcept>
#include <cmath>

// @implements SPEC-TL-DECLARATION
// @spec Declaration
namespace tela {
void Document::append(const std::string& id, ElementKind kind,
                      const std::string& label, InputPolicy input, Layout layout,
                      std::function<void()> action) {
    for (float v : {layout.width, layout.height, layout.padding, layout.gap})
        if (!std::isfinite(v) || v < 0 || v > 32768)
            throw std::invalid_argument("Invalid Tela layout dimension");
    if (!std::isfinite(layout.x) || !std::isfinite(layout.y))
        throw std::invalid_argument("Invalid Tela layout position");
    if (elements_.size() >= 4096 || id.size() > 512 || label.size() > 16384)
        throw std::invalid_argument("Tela document limit exceeded");
    if (id.empty() || std::any_of(elements_.begin(), elements_.end(),
        [&](const Element& element) { return element.id == id; })) {
        throw std::invalid_argument("Tela element ID must be nonempty and unique");
    }
    elements_.push_back({id, parent_, kind, label, input, layout, std::move(action)});
}
void Document::panel(const std::string& id, const std::function<void()>& children, Layout layout) {
    if (!children) throw std::invalid_argument("Tela panel requires a declaration");
    std::string ancestor=parent_;unsigned depth=0;
    while(!ancestor.empty()){
        if(++depth>=64)throw std::invalid_argument("Tela panel nesting exceeds 64");
        auto found=std::find_if(elements_.begin(),elements_.end(),[&](const auto& e){return e.id==ancestor;});
        ancestor=found->parent;
    }
    const auto count = elements_.size();
    const auto previous = parent_;
    append(id, ElementKind::panel, {}, InputPolicy::passthrough, layout);
    parent_ = id;
    try {
        children();
    } catch (...) {
        parent_ = previous;
        elements_.resize(count);
        throw;
    }
    parent_ = previous;
}
void Document::text(const std::string& id, const std::string& value, Layout layout, InputPolicy input) {
    append(id, ElementKind::text, value, input, layout);
}
void Document::button(const std::string& id, const std::string& label,
                      std::function<void()> action, Layout layout, InputPolicy input) {
    append(id, ElementKind::button, label, input, layout, std::move(action));
}
} // namespace tela
