// @implements SPEC-TL-LIST-VIEW
// @spec List view
#pragma once
#include <cstddef>
#include <string_view>

namespace tela {
// Byte length of the canonical UTF-8 character starting at `at`, or 0 when it is not one
// (bad lead byte, truncated sequence, overlong form, surrogate or a value above U+10FFFF).
// Callers decide what an invalid character means for their own input.
std::size_t utf8_character_bytes(std::string_view text, std::size_t at) noexcept;
// True when the whole text is canonical UTF-8.
bool is_valid_utf8(std::string_view text) noexcept;
}
