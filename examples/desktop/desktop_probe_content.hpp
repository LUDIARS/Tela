#pragma once
#include <tela/desktop_placement.hpp>
#include <tela/runtime.hpp>
#include <optional>
#include <string>
// @spec Desktop overlay
// Fixed declaration for the desktop surface probe: a translucent rounded backdrop the declaration
// draws itself, one button, and a grip. Everything else lets clicks through to the desktop.
inline constexpr char desktop_probe_grip[] = "desktop-probe.grip";
inline constexpr float desktop_probe_width = 360, desktop_probe_height = 216;

class DesktopProbeContent {
public:
    // Redeclares only when something the declaration shows changed.
    void refresh(tela::Runtime&, const std::string& hovered);
    // Shows where the surface reported it settled after a drag.
    void moved(const tela::DesktopPlacement&);
private:
    struct Shown {
        int clicks;
        std::string placed, hovered;
        bool operator==(const Shown&) const = default;
    };
    int clicks_{};
    std::string placed_{"Drag the grip at the top right to move"};
    std::optional<Shown> declared_;
};
