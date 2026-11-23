#pragma once
// @implements SPEC-TL-WEB
// @spec Web rendering
#include <tela/web_session.hpp>
#include <optional>
#include <string>
#include <variant>

namespace tela::web {
// Browser coordinates are CSS pixels of the canvas, which are Tela's logical pixels.
struct WebPointer {
    PointerPhase phase{};
    PointerButton button{};
    float x{}, y{}, wheel_x{}, wheel_y{};
    std::uint32_t modifiers{};
};
struct WebKey { Key key{}; bool pressed{}; std::uint32_t modifiers{}; };
struct WebText { std::string text; };
struct WebResize { float width{}, height{}, dpi{1}; };
struct WebFocus { bool focused{}; };
using WebInput = std::variant<WebPointer, WebKey, WebText, WebResize, WebFocus>;
struct Decoded {
    std::optional<WebInput> input;
    std::string error;
};
Decoded decode_input(std::string_view message, const WebLimits&);
}
