// @spec SPEC-TL-SOURCE-VIEW
#include "../examples/windows/source_options.hpp"
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {
SourceOptions parse(std::vector<std::wstring> values) {
    std::vector<wchar_t*> pointers;
    for(auto& value : values) pointers.push_back(value.data());
    return source_options(static_cast<int>(pointers.size()), pointers.data());
}
void require(bool value, const char* why) { if(!value) throw std::runtime_error(why); }
}
int main() {
    try {
        const auto result = parse({L"tela_source", L"--font", L"font.ttf", L"--source", L"code file.cpp", L"--line", L"42"});
        require(result.line == 42 && result.source == L"code file.cpp", "path and line are preserved");
        for(const auto& number : {L"0", L"-1", L"2x", L"200001", L"999999999999999999999999"}) {
            bool rejected = false;
            try { parse({L"tool", L"--font", L"font.ttf", L"--source", L"code.cpp", L"--line", number}); }
            catch(const std::invalid_argument&) { rejected = true; }
            require(rejected, "invalid line accepted");
        }
        bool missing = false;
        try { parse({L"tool", L"--source", L"code.cpp"}); } catch(const std::invalid_argument&) { missing = true; }
        require(missing, "font is explicitly required");
    } catch(const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
