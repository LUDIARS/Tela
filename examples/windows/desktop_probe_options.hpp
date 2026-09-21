#pragma once
#include <tela/desktop_placement.hpp>
#include <string>
// @spec Desktop overlay
// A probe run shows the fixed probe declaration in one corner for a bounded time, so a visual
// check never leaves a surface behind.
struct DesktopProbeOptions {
    std::string font;
    tela::DesktopCorner corner{tela::DesktopCorner::bottom_right};
    int seconds{120};
};
// Throws std::invalid_argument for a missing font, an unknown option or a value out of range.
DesktopProbeOptions desktopProbeOptions(int argc, char** argv);
