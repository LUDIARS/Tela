#pragma once
// @implements SPEC-TL-WEB
// @spec Web rendering
#include <tela/geometry.hpp>
#include <cstdint>
#include <string>
#include <string_view>

namespace tela::web {
// Appends compact JSON. The caller keeps the structure balanced; commas are inserted here.
class JsonWriter {
public:
    JsonWriter& begin_object();
    JsonWriter& end_object();
    JsonWriter& begin_array();
    JsonWriter& end_array();
    JsonWriter& key(std::string_view);
    JsonWriter& string(std::string_view);
    JsonWriter& number(double);
    JsonWriter& integer(std::uint64_t);
    JsonWriter& boolean(bool);
    JsonWriter& null();
    JsonWriter& color(Color);
    JsonWriter& rect(Rect);
    // Inserts an already encoded value.
    JsonWriter& raw(std::string_view encoded);
    const std::string& text() const noexcept { return out_; }
    std::size_t size() const noexcept { return out_.size(); }
    std::string release() { return std::move(out_); }
private:
    void separate();
    std::string out_;
    bool first_{true}, after_key_{};
};
}
