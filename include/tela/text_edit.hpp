// @implements SPEC-TL-CONTROLS
// @spec Controls
#pragma once
#include <cstddef>
#include <string>
#include <string_view>

// Pure editing rules for text fields and areas. Offsets are UTF-8 byte offsets that always sit
// on a character boundary; nothing here knows fonts, widgets or the runtime.
namespace tela::text_edit {
struct Selection {
    std::size_t caret{}, anchor{};
    std::size_t start() const noexcept { return caret < anchor ? caret : anchor; }
    std::size_t end() const noexcept { return caret < anchor ? anchor : caret; }
    bool empty() const noexcept { return caret == anchor; }
};
struct Edit {
    std::string value;
    std::size_t caret{};
};
// Nearest character boundary at or before `at` (clamped to the text).
std::size_t clamp(std::string_view text, std::size_t at) noexcept;
std::size_t previous(std::string_view text, std::size_t at) noexcept;
std::size_t next(std::string_view text, std::size_t at) noexcept;
std::size_t line_start(std::string_view text, std::size_t at) noexcept;
std::size_t line_end(std::string_view text, std::size_t at) noexcept;
// Zero-based line of `at`, counting '\n'.
std::size_t line_index(std::string_view text, std::size_t at) noexcept;
// Moves by whole lines keeping the character column; stops at the first/last line.
std::size_t vertical(std::string_view text, std::size_t at, int lines) noexcept;
// Code points before `bytes` and the byte offset of the `characters`-th code point.
std::size_t characters(std::string_view text, std::size_t bytes) noexcept;
std::size_t byte_offset(std::string_view text, std::size_t characters) noexcept;
// Replaces the selection with `insert`, truncating the insertion at a character boundary so
// the result stays within `max_bytes`.
Edit replace(std::string_view value, Selection, std::string_view insert, std::size_t max_bytes);
// Deletes the selection, or one character before (backward) / after (forward) the caret.
Edit erase(std::string_view value, Selection, bool forward);
// Committed text reduced to what a field accepts: no control characters, and '\n' only when
// multiline ("\r\n" and "\r" become '\n'). Invalid UTF-8 yields an empty string.
std::string sanitize(std::string_view text, bool multiline);
}
