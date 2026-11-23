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
enum class ElementKind {
    panel, text, button, canvas,
    // Controls (SPEC-TL-CONTROLS). Their values belong to the application.
    text_field, text_area, slider, list_item, separator, status, scroll,
    // Media placeholder (SPEC-TL-WEB). Browsers overlay an embedder-supplied stream.
    video
};
// Good / bad mark of a status line. Neutral draws the muted mark.
enum class Tone { neutral, good, warning, bad };
// Visible value of a control. The application owns it and redeclares it after a change;
// the runtime only keeps caret, selection and scroll position by element ID.
struct ElementValue {
    std::string text;   // field contents, list-item detail
    float fraction{};   // slider position in [0,1]
    bool secret{};      // text field shows a bullet per character
    bool selected{};    // list item mark
    Tone tone{Tone::neutral};
    std::string stream; // video: key the embedder resolves to a media stream
    bool operator==(const ElementValue&) const = default;
};
struct Element {
    std::string id;
    std::string parent;
    ElementKind kind;
    std::string label;
    InputPolicy input;
    Layout layout;
    std::function<void()> action; // button / list item activation, text field submit
    Drawing drawing;
    ElementValue value;
    std::function<void(const std::string&)> on_change; // text field / text area edit
    std::function<void(float)> on_set;                   // slider move
};
// Single-line text input. `label` of the element is the placeholder shown while empty.
struct TextFieldOptions {
    std::string placeholder;
    bool secret{};
    std::function<void(const std::string&)> on_change;
    std::function<void()> on_submit;
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
    // Controls. Inputs default to exclusive; marks and lines are draw-only.
    void text_field(const std::string& id, const std::string& value, TextFieldOptions options,
                    Layout layout = {});
    // Multi-line input; `layout.lines` is the number of visible rows. Lines break only at '\n'.
    void text_area(const std::string& id, const std::string& value,
                   std::function<void(const std::string&)> on_change, Layout layout = {.lines = 4});
    void slider(const std::string& id, float fraction, std::function<void(float)> on_set,
                Layout layout = {});
    void list_item(const std::string& id, const std::string& label, const std::string& detail,
                   bool selected, std::function<void()> action = {}, Layout layout = {});
    void separator(const std::string& id, Layout layout = {});
    void status(const std::string& id, const std::string& label, Tone tone, Layout layout = {});
    // Clips its children and scrolls them vertically. Zero height fills the parent's remaining height.
    void scroll(const std::string& id, const std::function<void()>& children, Layout layout = {});
    // Browser hosts overlay the stream registered under `stream`; native hosts draw a frame and label.
    void video(const std::string& id, const std::string& label, const std::string& stream,
               Layout layout = {});
private:
    void append(const std::string& id, ElementKind kind,
                const std::string& label, InputPolicy input, Layout layout,
                std::function<void()> action = {});
    void container(const std::string& id, ElementKind kind, const std::function<void()>& children,
                   Layout layout, InputPolicy input);
    std::vector<Element> elements_;
    std::string parent_;
};
} // namespace tela
