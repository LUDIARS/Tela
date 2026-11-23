// @spec SPEC-TL-CONTROLS
#include <tela/document.hpp>
#include <tela/utf8.hpp>
#include <algorithm>
#include <cmath>
#include <stdexcept>

// @implements SPEC-TL-CONTROLS
// @spec Controls
namespace tela {
namespace {
constexpr std::size_t value_limit = 16384;
constexpr std::size_t stream_limit = 512;

void text_value(const std::string& value) {
    if (value.size() > value_limit) throw std::invalid_argument("Tela control value exceeds 16384 bytes");
    if (!is_valid_utf8(value)) throw std::invalid_argument("Tela control value must be UTF-8");
}
}

void Document::text_field(const std::string& id, const std::string& value, TextFieldOptions options,
                          Layout layout) {
    text_value(value);
    if (value.find('\n') != std::string::npos) throw std::invalid_argument("Tela text field is a single line");
    append(id, ElementKind::text_field, options.placeholder, InputPolicy::exclusive, layout,
           std::move(options.on_submit));
    auto& element = elements_.back();
    element.value.text = value;
    element.value.secret = options.secret;
    element.on_change = std::move(options.on_change);
}
void Document::text_area(const std::string& id, const std::string& value,
                         std::function<void(const std::string&)> on_change, Layout layout) {
    text_value(value);
    append(id, ElementKind::text_area, {}, InputPolicy::exclusive, layout);
    elements_.back().value.text = value;
    elements_.back().on_change = std::move(on_change);
}
void Document::slider(const std::string& id, float fraction, std::function<void(float)> on_set,
                      Layout layout) {
    if (!std::isfinite(fraction)) throw std::invalid_argument("Tela slider fraction must be finite");
    append(id, ElementKind::slider, {}, InputPolicy::exclusive, layout);
    elements_.back().value.fraction = std::clamp(fraction, 0.f, 1.f);
    elements_.back().on_set = std::move(on_set);
}
void Document::list_item(const std::string& id, const std::string& label, const std::string& detail,
                         bool selected, std::function<void()> action, Layout layout) {
    text_value(detail);
    append(id, ElementKind::list_item, label, InputPolicy::exclusive, layout, std::move(action));
    elements_.back().value.text = detail;
    elements_.back().value.selected = selected;
}
void Document::separator(const std::string& id, Layout layout) {
    append(id, ElementKind::separator, {}, InputPolicy::passthrough, layout);
}
void Document::status(const std::string& id, const std::string& label, Tone tone, Layout layout) {
    if (tone < Tone::neutral || tone > Tone::bad) throw std::invalid_argument("Invalid Tela status tone");
    append(id, ElementKind::status, label, InputPolicy::passthrough, layout);
    elements_.back().value.tone = tone;
}
void Document::scroll(const std::string& id, const std::function<void()>& children, Layout layout) {
    // Exclusive so the wheel over its blank area reaches the scroll instead of the host.
    container(id, ElementKind::scroll, children, layout, InputPolicy::exclusive);
}
void Document::video(const std::string& id, const std::string& label, const std::string& stream,
                     Layout layout) {
    if (stream.size() > stream_limit || !is_valid_utf8(stream))
        throw std::invalid_argument("Tela video stream key must be UTF-8 of at most 512 bytes");
    append(id, ElementKind::video, label, InputPolicy::passthrough, layout);
    elements_.back().value.stream = stream;
}
} // namespace tela
