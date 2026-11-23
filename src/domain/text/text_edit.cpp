// @spec SPEC-TL-CONTROLS
#include <tela/text_edit.hpp>
#include <tela/utf8.hpp>
#include <algorithm>

namespace tela::text_edit {
namespace {
bool continuation(char c) noexcept { return (static_cast<unsigned char>(c) & 0xC0) == 0x80; }
}
std::size_t clamp(std::string_view text, std::size_t at) noexcept {
    at = std::min(at, text.size());
    while (at > 0 && at < text.size() && continuation(text[at])) --at;
    return at;
}
std::size_t previous(std::string_view text, std::size_t at) noexcept {
    at = clamp(text, at);
    if (at == 0) return 0;
    --at;
    while (at > 0 && continuation(text[at])) --at;
    return at;
}
std::size_t next(std::string_view text, std::size_t at) noexcept {
    at = clamp(text, at);
    if (at >= text.size()) return text.size();
    ++at;
    while (at < text.size() && continuation(text[at])) ++at;
    return at;
}
std::size_t line_start(std::string_view text, std::size_t at) noexcept {
    at = clamp(text, at);
    const auto found = at == 0 ? std::string_view::npos : text.rfind('\n', at - 1);
    return found == std::string_view::npos ? 0 : found + 1;
}
std::size_t line_end(std::string_view text, std::size_t at) noexcept {
    const auto found = text.find('\n', clamp(text, at));
    return found == std::string_view::npos ? text.size() : found;
}
std::size_t line_index(std::string_view text, std::size_t at) noexcept {
    at = clamp(text, at);
    return static_cast<std::size_t>(std::count(text.begin(), text.begin() + static_cast<std::ptrdiff_t>(at), '\n'));
}
std::size_t vertical(std::string_view text, std::size_t at, int lines) noexcept {
    at = clamp(text, at);
    const auto start = line_start(text, at);
    const auto column = characters(text.substr(start), at - start);
    auto line = start;
    for (; lines < 0; ++lines) {
        if (line == 0) return 0;
        line = line_start(text, line - 1);
    }
    for (; lines > 0; --lines) {
        const auto end = line_end(text, line);
        if (end >= text.size()) return text.size();
        line = end + 1;
    }
    const auto end = line_end(text, line);
    return line + byte_offset(text.substr(line, end - line), column);
}
std::size_t characters(std::string_view text, std::size_t bytes) noexcept {
    bytes = clamp(text, bytes);
    std::size_t count = 0;
    for (std::size_t i = 0; i < bytes; ++i) if (!continuation(text[i])) ++count;
    return count;
}
std::size_t byte_offset(std::string_view text, std::size_t count) noexcept {
    std::size_t at = 0;
    while (count > 0 && at < text.size()) { at = next(text, at); --count; }
    return at;
}
Edit replace(std::string_view value, Selection selection, std::string_view insert, std::size_t max_bytes) {
    const auto start = clamp(value, selection.start()), end = clamp(value, selection.end());
    const auto kept = value.size() - (end - start);
    std::size_t room = max_bytes > kept ? max_bytes - kept : 0;
    if (insert.size() > room) {
        // Cut at a boundary inside the insertion so a multi-byte character is never split.
        while (room > 0 && room < insert.size() && continuation(insert[room])) --room;
        insert = insert.substr(0, room);
    }
    Edit result;
    result.value.reserve(kept + insert.size());
    result.value.append(value.substr(0, start)).append(insert).append(value.substr(end));
    result.caret = start + insert.size();
    return result;
}
Edit erase(std::string_view value, Selection selection, bool forward) {
    if (!selection.empty()) return replace(value, selection, {}, value.size());
    const auto caret = clamp(value, selection.caret);
    const Selection one = forward ? Selection{next(value, caret), caret} : Selection{previous(value, caret), caret};
    return replace(value, one, {}, value.size());
}
std::string sanitize(std::string_view text, bool multiline) {
    if (!is_valid_utf8(text)) return {};
    std::string result;
    result.reserve(text.size());
    for (std::size_t i = 0; i < text.size(); ++i) {
        const auto c = static_cast<unsigned char>(text[i]);
        if (c == '\r') {
            if (multiline) result.push_back('\n');
            if (i + 1 < text.size() && text[i + 1] == '\n') ++i;
            continue;
        }
        if (c == '\n') { if (multiline) result.push_back('\n'); continue; }
        if (c < 0x20 || c == 0x7F) continue;
        result.push_back(static_cast<char>(c));
    }
    return result;
}
}
