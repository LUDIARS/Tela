#include <tela/document.hpp>
#include <algorithm>
#include <stdexcept>

// @implements SPEC-TL-DECLARATION
namespace tela {
void Document::append(const std::string& id, ElementKind kind,
                      const std::string& label, InputPolicy input) {
    if (id.empty() || std::any_of(elements_.begin(), elements_.end(),
        [&](const Element& element) { return element.id == id; })) {
        throw std::invalid_argument("Tela element ID must be nonempty and unique");
    }
    elements_.push_back({id, parent_, kind, label, input});
}
void Document::panel(const std::string& id, const std::function<void()>& children) {
    if (!children) throw std::invalid_argument("Tela panel requires a declaration");
    const auto count = elements_.size();
    const auto previous = parent_;
    append(id, ElementKind::panel, {}, InputPolicy::passthrough);
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
void Document::text(const std::string& id, const std::string& value) {
    append(id, ElementKind::text, value, InputPolicy::passthrough);
}
void Document::button(const std::string& id, const std::string& label) {
    append(id, ElementKind::button, label, InputPolicy::exclusive);
}
} // namespace tela
