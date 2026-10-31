// @implements SPEC-TL-LIST-VIEW
// @spec List view
#pragma once
#include <tela/desktop_placement.hpp>
#include <cstddef>
#include <optional>
#include <string>
// A list overlay run floats one `TELA_LIST_VIEW 1` file on the desktop and redraws it when the
// writer replaces the file. It runs until it is stopped, unless --seconds bounds a check.
struct ListOverlayOptions {
    std::string font;
    std::string file;
    tela::DesktopCorner corner{tela::DesktopCorner::top_right};
    float width{520};
    std::size_t rows{12};
    std::optional<float> font_size;
    // 0 = until the process is stopped.
    int seconds{};
    bool exclude_from_capture{};
};
// Throws std::invalid_argument for a missing font or file, an unknown option or a value out of range.
ListOverlayOptions listOverlayOptions(int argc, char** argv);
