// @implements SPEC-TL-LIST-VIEW
// @spec List view
#include <tela/utf8.hpp>

namespace tela {
std::size_t utf8_character_bytes(std::string_view text, std::size_t at) noexcept {
    if(at >= text.size()) return 0;
    const auto lead = static_cast<unsigned char>(text[at]);
    if(lead < 0x80) return 1;
    const std::size_t count = lead >= 0xc2 && lead <= 0xdf ? 2 :
        lead >= 0xe0 && lead <= 0xef ? 3 : lead >= 0xf0 && lead <= 0xf4 ? 4 : 0;
    if(!count || at + count > text.size()) return 0;
    unsigned code = lead & (0x7f >> count);
    for(std::size_t i = 1; i < count; ++i) {
        const auto next = static_cast<unsigned char>(text[at + i]);
        if((next & 0xc0) != 0x80) return 0;
        code = (code << 6) | (next & 0x3f);
    }
    if((count == 2 && code < 0x80) || (count == 3 && code < 0x800) ||
       (count == 4 && code < 0x10000) || code > 0x10ffff || (code >= 0xd800 && code <= 0xdfff))
        return 0;
    return count;
}

bool is_valid_utf8(std::string_view text) noexcept {
    for(std::size_t at = 0; at < text.size();) {
        const auto length = utf8_character_bytes(text, at);
        if(!length) return false;
        at += length;
    }
    return true;
}
}
