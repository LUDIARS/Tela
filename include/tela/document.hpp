// @implements SPEC-TL-DECLARATION
// @spec Declaration
#pragma once
#include <functional>
#include <string>
#include <vector>
#include <tela/geometry.hpp>
#include <tela/drawing.hpp>

namespace tela {
enum class InputPolicy { passthrough, exclusive, shared };
enum class ElementKind { panel, text, button, canvas };
struct Element {
    std::string id;
    std::string parent;
    ElementKind kind;
    std::string label;
    InputPolicy input;
    Layout layout;
    std::function<void()> action;
    Drawing drawing;
};

// GPU-independent declaration builder; stable IDs belong to the application.
class Document {
public:
    void panel(const std::string& id, const std::function<void()>& children, Layout layout = {});
    void text(const std::string& id, const std::string& value, Layout layout = {},
              InputPolicy input = InputPolicy::passthrough);
    void button(const std::string& id, const std::string& label,
                std::function<void()> action = {}, Layout layout = {},
                InputPolicy input = InputPolicy::exclusive);
    const std::vector<Element>& elements() const noexcept { return elements_; }
    // Draw-only by default, so callout lines never steal host input.
    void canvas(const std::string& id, Drawing drawing, Layout layout,
                InputPolicy input = InputPolicy::passthrough);
private:
    void append(const std::string& id, ElementKind kind,
                const std::string& label, InputPolicy input, Layout layout,
                std::function<void()> action = {});
    std::vector<Element> elements_;
    std::string parent_;
};
} // namespace tela
