// @spec SPEC-TL-SOURCE-VIEW
#pragma once
#include <filesystem>
#include <string>

struct SourceOptions {
    std::filesystem::path source;
    std::string font;
    std::size_t line{1};
    int font_size{16};
};
SourceOptions source_options(int argc, wchar_t** argv);
