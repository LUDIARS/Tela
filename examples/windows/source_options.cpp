// @spec SPEC-TL-SOURCE-VIEW
#include "source_options.hpp"
#include <stdexcept>
#include <string_view>

namespace {
std::size_t number(std::wstring_view value, std::size_t maximum) {
    if(value.empty()) throw std::invalid_argument("Empty source viewer number");
    std::size_t result = 0;
    for(const auto ch : value) {
        if(ch < L'0' || ch > L'9' || result > (maximum - static_cast<unsigned>(ch - L'0')) / 10)
            throw std::invalid_argument("Invalid source viewer number");
        result = result * 10 + static_cast<unsigned>(ch - L'0');
    }
    if(!result || result > maximum) throw std::invalid_argument("Source viewer number is out of range");
    return result;
}
}
SourceOptions source_options(int argc, wchar_t** argv) {
    SourceOptions result;
    bool source_seen = false, font_seen = false, line_seen = false, size_seen = false;
    for(int i = 1; i < argc; ++i) {
        const std::wstring_view option(argv[i]);
        if(i + 1 >= argc) throw std::invalid_argument("Missing source viewer option value");
        const std::wstring_view value(argv[++i]);
        if(value.empty()) throw std::invalid_argument("Empty source viewer option value");
        if(option == L"--source" && !source_seen) {
            result.source = value; source_seen = true;
        } else if(option == L"--font" && !font_seen) {
            const auto utf8 = std::filesystem::path(value).u8string();
            result.font.assign(reinterpret_cast<const char*>(utf8.data()), utf8.size()); font_seen = true;
        } else if(option == L"--line" && !line_seen) {
            result.line = number(value, 200000); line_seen = true;
        } else if(option == L"--font-size" && !size_seen) {
            result.font_size = static_cast<int>(number(value, 48)); size_seen = true;
            if(result.font_size < 8) throw std::invalid_argument("Font size must be 8..48");
        } else throw std::invalid_argument("Unknown or duplicate source viewer option");
    }
    if(!font_seen || !source_seen)
        throw std::invalid_argument("Usage: tela_source --font <monospace.ttf> --source <file> [--line N] [--font-size N]");
    return result;
}
