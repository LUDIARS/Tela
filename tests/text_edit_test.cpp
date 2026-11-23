// @implements SPEC-TL-CONTROLS
// @spec Controls
#include <tela/text_edit.hpp>
#include <iostream>
#include <stdexcept>

namespace {
void require(bool condition, const char* message) { if (!condition) throw std::runtime_error(message); }
}
int main() { try {
    namespace te = tela::text_edit;
    const std::string mixed = "a\xE3\x81\x82" "b"; // a, U+3042, b
    require(te::next(mixed, 1) == 4 && te::previous(mixed, 4) == 1, "caret steps over a whole multi-byte character");
    require(te::clamp(mixed, 2) == 1 && te::clamp(mixed, 99) == mixed.size(), "offsets snap back to a character boundary");
    require(te::previous(mixed, 0) == 0 && te::next(mixed, mixed.size()) == mixed.size(), "caret stops at the ends");
    require(te::characters(mixed, 4) == 2 && te::byte_offset(mixed, 2) == 4, "code point and byte offsets convert both ways");

    const std::string lines = "abc\nd\nefgh";
    require(te::line_start(lines, 5) == 4 && te::line_end(lines, 4) == 5, "line bounds follow newlines");
    require(te::line_index(lines, 7) == 2, "line index counts newlines before the caret");
    require(te::vertical(lines, 2, 1) == 5, "moving down keeps the column but stops at a short line's end");
    require(te::vertical(lines, 8, -1) == 5 && te::vertical(lines, 8, -2) == 2, "moving up keeps the column");
    require(te::vertical(lines, 1, -1) == 0 && te::vertical(lines, 7, 1) == lines.size(), "vertical moves stop at the first and last line");

    auto edit = te::replace("hello", {5, 5}, " world", 64);
    require(edit.value == "hello world" && edit.caret == 11, "insertion at the caret");
    edit = te::replace("hello", {1, 4}, "EY", 64);
    require(edit.value == "hEYo" && edit.caret == 3, "insertion replaces the selection whichever way it was made");
    edit = te::replace("ab", {2, 2}, "\xE3\x81\x82\xE3\x81\x84", 6);
    require(edit.value == "ab\xE3\x81\x82" && edit.caret == 5, "insertion is cut at a character boundary to fit the limit");
    edit = te::erase(mixed, {4, 4}, false);
    require(edit.value == "ab" && edit.caret == 1, "backspace removes one whole character");
    edit = te::erase(mixed, {1, 1}, true);
    require(edit.value == "ab" && edit.caret == 1, "delete removes the character after the caret");
    edit = te::erase("abcd", {3, 1}, false);
    require(edit.value == "ad" && edit.caret == 1, "erase removes the selection first");

    require(te::sanitize("a\tb\x01" "c\x7F", false) == "abc", "control characters are dropped");
    require(te::sanitize("1\r\n2\r3\n4", true) == "1\n2\n3\n4", "a text area normalizes line breaks");
    require(te::sanitize("1\r\n2", false) == "12", "a text field drops line breaks");
    require(te::sanitize("\xC3", true).empty(), "invalid UTF-8 is refused as a whole");
    return 0;
} catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; } }
