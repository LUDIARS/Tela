#pragma once
// @implements SPEC-TL-WEB
// @spec Web rendering
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace tela::web {
// Bounded JSON value for browser input. Objects keep keys and values in parallel arrays.
struct JsonValue {
    enum class Type { null, boolean, number, string, array, object };
    Type type{Type::null};
    bool boolean{};
    double number{};
    std::string text;
    std::vector<std::string> keys;
    std::vector<JsonValue> items; // array items, or object values matching `keys`
    const JsonValue* find(std::string_view key) const noexcept;
};
struct JsonLimits {
    std::size_t depth{4}, nodes{64}, string_bytes{4096};
};
// Strict RFC 8259 parse; nullopt for malformed input, invalid UTF-8 or a limit overrun.
std::optional<JsonValue> parse_json(std::string_view text, JsonLimits limits = {});
}
