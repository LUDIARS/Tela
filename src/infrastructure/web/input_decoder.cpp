// @spec SPEC-TL-WEB
#include "input_decoder.hpp"
#include "json_reader.hpp"
#include <array>
#include <cmath>
#include <utility>

namespace tela::web {
namespace {
constexpr float coordinate_limit = 65536, wheel_limit = 100;

template <typename T, std::size_t N>
std::optional<T> lookup(const std::array<std::pair<std::string_view, T>, N>& table, std::string_view name) {
    for (const auto& [key, value] : table) if (key == name) return value;
    return std::nullopt;
}
constexpr std::array<std::pair<std::string_view, PointerPhase>, 5> phases{{
    {"move", PointerPhase::move}, {"down", PointerPhase::down}, {"up", PointerPhase::up},
    {"cancel", PointerPhase::cancel}, {"wheel", PointerPhase::wheel}}};
constexpr std::array<std::pair<std::string_view, PointerButton>, 4> buttons{{
    {"none", PointerButton::none}, {"primary", PointerButton::primary},
    {"secondary", PointerButton::secondary}, {"middle", PointerButton::middle}}};
// DOM KeyboardEvent.key names.
constexpr std::array<std::pair<std::string_view, Key>, 15> keys{{
    {"Tab", Key::tab}, {"Enter", Key::enter}, {"Escape", Key::escape}, {"Backspace", Key::backspace},
    {"Delete", Key::delete_forward}, {"ArrowLeft", Key::left}, {"ArrowRight", Key::right},
    {"ArrowUp", Key::up}, {"ArrowDown", Key::down}, {"Home", Key::home}, {"End", Key::end},
    {"PageUp", Key::page_up}, {"PageDown", Key::page_down}, {"a", Key::a}, {"A", Key::a}}};

class Fields {
public:
    explicit Fields(const JsonValue& object) : object_(object) {}
    std::string error;
    std::optional<std::string_view> name(std::string_view key) {
        const auto* v = object_.find(key);
        if (!v || v->type != JsonValue::Type::string) { fail(key); return std::nullopt; }
        return std::string_view(v->text);
    }
    float number(std::string_view key, float low, float high, std::optional<float> fallback = std::nullopt) {
        const auto* v = object_.find(key);
        if (!v && fallback) return *fallback;
        if (!v || v->type != JsonValue::Type::number || v->number < low || v->number > high) { fail(key); return 0; }
        return static_cast<float>(v->number);
    }
    bool flag(std::string_view key) {
        const auto* v = object_.find(key);
        if (!v || v->type != JsonValue::Type::boolean) { fail(key); return false; }
        return v->boolean;
    }
    std::uint32_t modifiers() {
        const float bits = number("modifiers", 0, 15, 0.f);
        if (bits != std::floor(bits)) fail("modifiers");
        return static_cast<std::uint32_t>(bits);
    }
private:
    void fail(std::string_view key) { if (error.empty()) error = "invalid field: " + std::string(key); }
    const JsonValue& object_;
};
Decoded result(Fields& fields, WebInput input) {
    if (!fields.error.empty()) return {std::nullopt, fields.error};
    return {std::move(input), {}};
}
}
Decoded decode_input(std::string_view message, const WebLimits& limits) {
    if (message.size() > limits.input_bytes) return {std::nullopt, "input message exceeds limit"};
    const auto parsed = parse_json(message, {4, 64, limits.text_bytes});
    if (!parsed || parsed->type != JsonValue::Type::object) return {std::nullopt, "input is not a bounded JSON object"};
    Fields fields(*parsed);
    const auto type = fields.name("type");
    if (!type) return {std::nullopt, fields.error};
    if (*type == "pointer") {
        WebPointer p;
        const auto phase = fields.name("phase");
        const auto button = parsed->find("button") ? fields.name("button") : std::optional<std::string_view>("none");
        const auto known_phase = phase ? lookup(phases, *phase) : std::nullopt;
        const auto known_button = button ? lookup(buttons, *button) : std::nullopt;
        if (!known_phase || !known_button) return {std::nullopt, "unknown pointer phase or button"};
        p.phase = *known_phase; p.button = *known_button;
        p.x = fields.number("x", -coordinate_limit, coordinate_limit);
        p.y = fields.number("y", -coordinate_limit, coordinate_limit);
        p.wheel_x = fields.number("wheel_x", -wheel_limit, wheel_limit, 0.f);
        p.wheel_y = fields.number("wheel_y", -wheel_limit, wheel_limit, 0.f);
        p.modifiers = fields.modifiers();
        return result(fields, p);
    }
    if (*type == "key") {
        const auto name = fields.name("key");
        const auto key = name ? lookup(keys, *name) : std::nullopt;
        if (!key) return {std::nullopt, "unknown key"};
        WebKey k{*key, fields.flag("pressed"), fields.modifiers()};
        return result(fields, k);
    }
    if (*type == "text") {
        const auto text = fields.name("text");
        if (!text) return {std::nullopt, fields.error};
        return result(fields, WebText{std::string(*text)});
    }
    if (*type == "resize") {
        WebResize r;
        r.width = fields.number("width", 0, 16384);
        r.height = fields.number("height", 0, 16384);
        r.dpi = fields.number("dpi", .25f, 8, 1.f);
        return result(fields, r);
    }
    if (*type == "focus") return result(fields, WebFocus{fields.flag("focused")});
    return {std::nullopt, "unknown input type"};
}
}
