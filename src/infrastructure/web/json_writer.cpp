// @spec SPEC-TL-WEB
#include "json_writer.hpp"
#include <cmath>
#include <cstdio>

namespace tela::web {
void JsonWriter::separate() {
    if (after_key_) { after_key_ = false; return; }
    if (!first_) out_.push_back(',');
    first_ = false;
}
JsonWriter& JsonWriter::begin_object() { separate(); out_.push_back('{'); first_ = true; return *this; }
JsonWriter& JsonWriter::end_object() { out_.push_back('}'); first_ = false; return *this; }
JsonWriter& JsonWriter::begin_array() { separate(); out_.push_back('['); first_ = true; return *this; }
JsonWriter& JsonWriter::end_array() { out_.push_back(']'); first_ = false; return *this; }
JsonWriter& JsonWriter::key(std::string_view name) {
    string(name);
    out_.push_back(':');
    after_key_ = true;
    return *this;
}
JsonWriter& JsonWriter::string(std::string_view value) {
    separate();
    out_.push_back('"');
    for (const char raw : value) {
        const auto c = static_cast<unsigned char>(raw);
        switch (c) {
        case '"': out_ += "\\\""; break;
        case '\\': out_ += "\\\\"; break;
        case '\n': out_ += "\\n"; break;
        case '\r': out_ += "\\r"; break;
        case '\t': out_ += "\\t"; break;
        default:
            if (c < 0x20) {
                char escaped[8];
                std::snprintf(escaped, sizeof escaped, "\\u%04x", c);
                out_ += escaped;
            } else {
                out_.push_back(raw); // UTF-8 passes through; values are validated at declaration
            }
        }
    }
    out_.push_back('"');
    return *this;
}
JsonWriter& JsonWriter::number(double value) {
    separate();
    if (!std::isfinite(value)) value = 0;
    // Three decimals are below a hundredth of a logical pixel; trailing zeros are dropped.
    char buffer[32];
    std::snprintf(buffer, sizeof buffer, "%.3f", value);
    std::string text = buffer;
    while (!text.empty() && text.back() == '0') text.pop_back();
    if (!text.empty() && text.back() == '.') text.pop_back();
    if (text == "-0") text = "0";
    out_ += text;
    return *this;
}
JsonWriter& JsonWriter::integer(std::uint64_t value) { separate(); out_ += std::to_string(value); return *this; }
JsonWriter& JsonWriter::boolean(bool value) { separate(); out_ += value ? "true" : "false"; return *this; }
JsonWriter& JsonWriter::raw(std::string_view encoded) { separate(); out_ += encoded; return *this; }
JsonWriter& JsonWriter::null() { separate(); out_ += "null"; return *this; }
JsonWriter& JsonWriter::color(Color c) {
    begin_array().integer(c.r).integer(c.g).integer(c.b).integer(c.a);
    return end_array();
}
JsonWriter& JsonWriter::rect(Rect r) {
    begin_array().number(r.x).number(r.y).number(r.width).number(r.height);
    return end_array();
}
}
