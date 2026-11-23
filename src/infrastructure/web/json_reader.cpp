// @spec SPEC-TL-WEB
#include "json_reader.hpp"
#include <tela/utf8.hpp>
#include <charconv>
#include <cmath>

namespace tela::web {
const JsonValue* JsonValue::find(std::string_view key) const noexcept {
    if (type != Type::object) return nullptr;
    for (std::size_t i = 0; i < keys.size(); ++i) if (keys[i] == key) return &items[i];
    return nullptr;
}
namespace {
class Parser {
public:
    Parser(std::string_view text, JsonLimits limits) : text_(text), limits_(limits) {}
    std::optional<JsonValue> run() {
        JsonValue value;
        if (!parse(value, 0)) return std::nullopt;
        space();
        if (at_ != text_.size()) return std::nullopt;
        return value;
    }
private:
    std::string_view text_;
    JsonLimits limits_;
    std::size_t at_{}, nodes_{};

    void space() {
        while (at_ < text_.size() && (text_[at_] == ' ' || text_[at_] == '\t' || text_[at_] == '\n' || text_[at_] == '\r')) ++at_;
    }
    bool literal(std::string_view word) {
        if (text_.substr(at_, word.size()) != word) return false;
        at_ += word.size();
        return true;
    }
    bool parse(JsonValue& out, std::size_t depth) {
        if (++nodes_ > limits_.nodes || depth > limits_.depth) return false;
        space();
        if (at_ >= text_.size()) return false;
        const char c = text_[at_];
        if (c == '{') return object(out, depth);
        if (c == '[') return array(out, depth);
        if (c == '"') { out.type = JsonValue::Type::string; return string(out.text); }
        if (c == 't' || c == 'f') { out.type = JsonValue::Type::boolean; out.boolean = c == 't'; return literal(c == 't' ? "true" : "false"); }
        if (c == 'n') { out.type = JsonValue::Type::null; return literal("null"); }
        return number(out);
    }
    bool object(JsonValue& out, std::size_t depth) {
        out.type = JsonValue::Type::object;
        ++at_;
        space();
        if (at_ < text_.size() && text_[at_] == '}') { ++at_; return true; }
        for (;;) {
            space();
            std::string key;
            if (at_ >= text_.size() || text_[at_] != '"' || !string(key)) return false;
            for (const auto& existing : out.keys) if (existing == key) return false; // duplicate keys are ambiguous
            space();
            if (at_ >= text_.size() || text_[at_++] != ':') return false;
            JsonValue value;
            if (!parse(value, depth + 1)) return false;
            out.keys.push_back(std::move(key));
            out.items.push_back(std::move(value));
            space();
            if (at_ >= text_.size()) return false;
            if (text_[at_] == '}') { ++at_; return true; }
            if (text_[at_++] != ',') return false;
        }
    }
    bool array(JsonValue& out, std::size_t depth) {
        out.type = JsonValue::Type::array;
        ++at_;
        space();
        if (at_ < text_.size() && text_[at_] == ']') { ++at_; return true; }
        for (;;) {
            JsonValue value;
            if (!parse(value, depth + 1)) return false;
            out.items.push_back(std::move(value));
            space();
            if (at_ >= text_.size()) return false;
            if (text_[at_] == ']') { ++at_; return true; }
            if (text_[at_++] != ',') return false;
        }
    }
    static int hex(char c) {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
    }
    bool unit(std::uint32_t& value) {
        if (at_ + 4 > text_.size()) return false;
        value = 0;
        for (int i = 0; i < 4; ++i) {
            const int digit = hex(text_[at_++]);
            if (digit < 0) return false;
            value = value * 16 + static_cast<std::uint32_t>(digit);
        }
        return true;
    }
    static void append(std::string& out, std::uint32_t cp) {
        if (cp < 0x80) out.push_back(static_cast<char>(cp));
        else if (cp < 0x800) { out.push_back(static_cast<char>(0xC0 | (cp >> 6))); out.push_back(static_cast<char>(0x80 | (cp & 0x3F))); }
        else if (cp < 0x10000) {
            out.push_back(static_cast<char>(0xE0 | (cp >> 12)));
            out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        } else {
            out.push_back(static_cast<char>(0xF0 | (cp >> 18)));
            out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        }
    }
    bool string(std::string& out) {
        ++at_; // opening quote
        for (;;) {
            if (at_ >= text_.size() || out.size() > limits_.string_bytes) return false;
            const char c = text_[at_++];
            if (c == '"') return out.size() <= limits_.string_bytes && is_valid_utf8(out);
            if (static_cast<unsigned char>(c) < 0x20) return false;
            if (c != '\\') { out.push_back(c); continue; }
            if (at_ >= text_.size()) return false;
            const char e = text_[at_++];
            switch (e) {
            case '"': out.push_back('"'); break;
            case '\\': out.push_back('\\'); break;
            case '/': out.push_back('/'); break;
            case 'b': out.push_back('\b'); break;
            case 'f': out.push_back('\f'); break;
            case 'n': out.push_back('\n'); break;
            case 'r': out.push_back('\r'); break;
            case 't': out.push_back('\t'); break;
            case 'u': {
                std::uint32_t cp = 0;
                if (!unit(cp)) return false;
                if (cp >= 0xDC00 && cp <= 0xDFFF) return false;
                if (cp >= 0xD800 && cp <= 0xDBFF) {
                    std::uint32_t low = 0;
                    if (!literal("\\u") || !unit(low) || low < 0xDC00 || low > 0xDFFF) return false;
                    cp = 0x10000 + ((cp - 0xD800) << 10) + (low - 0xDC00);
                }
                append(out, cp);
                break;
            }
            default: return false;
            }
        }
    }
    bool number(JsonValue& out) {
        const auto start = at_;
        if (at_ < text_.size() && text_[at_] == '-') ++at_;
        if (at_ >= text_.size() || text_[at_] < '0' || text_[at_] > '9') return false;
        if (text_[at_] == '0') ++at_;
        else while (at_ < text_.size() && text_[at_] >= '0' && text_[at_] <= '9') ++at_;
        if (at_ < text_.size() && text_[at_] == '.') {
            ++at_;
            if (at_ >= text_.size() || text_[at_] < '0' || text_[at_] > '9') return false;
            while (at_ < text_.size() && text_[at_] >= '0' && text_[at_] <= '9') ++at_;
        }
        if (at_ < text_.size() && (text_[at_] == 'e' || text_[at_] == 'E')) {
            ++at_;
            if (at_ < text_.size() && (text_[at_] == '+' || text_[at_] == '-')) ++at_;
            if (at_ >= text_.size() || text_[at_] < '0' || text_[at_] > '9') return false;
            while (at_ < text_.size() && text_[at_] >= '0' && text_[at_] <= '9') ++at_;
        }
        if (at_ - start > 32) return false;
        const auto token = text_.substr(start, at_ - start);
        const auto result = std::from_chars(token.data(), token.data() + token.size(), out.number);
        if (result.ec != std::errc{} || !std::isfinite(out.number)) return false;
        out.type = JsonValue::Type::number;
        return true;
    }
};
}
std::optional<JsonValue> parse_json(std::string_view text, JsonLimits limits) {
    return Parser(text, limits).run();
}
}
