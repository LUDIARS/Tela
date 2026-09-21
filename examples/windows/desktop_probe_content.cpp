// @spec Desktop overlay
#include "desktop_probe_content.hpp"
#include <cmath>

namespace {
// The backdrop's translucency is the declaration's own colour: black at 60 % alpha.
constexpr tela::Color backdrop{12, 14, 20, 153};
constexpr tela::Color edge{255, 255, 255, 64};
constexpr tela::Color grip_fill{255, 255, 255, 48};
constexpr tela::Color grip_mark{245, 246, 250, 230};
constexpr float inset = 16, grip_size = 28;

tela::Layout at(float x, float y, float width, unsigned lines = 1) {
    tela::Layout layout;
    layout.positioned = true; layout.x = x; layout.y = y; layout.width = width; layout.lines = lines;
    return layout;
}

tela::Drawing backdrop_drawing() {
    tela::Drawing drawing;
    drawing.rectangle({0, 0, desktop_probe_width, desktop_probe_height}, backdrop, {edge, 1}, 14);
    return drawing;
}

tela::Drawing grip_drawing() {
    tela::Drawing drawing;
    drawing.rectangle({0, 0, grip_size, grip_size}, grip_fill, {{0, 0, 0, 0}, 0}, 6);
    for(float row : {9.f, 14.f, 19.f}) drawing.line({8, row}, {grip_size - 8, row}, {grip_mark, 2});
    return drawing;
}
}

void DesktopProbeContent::moved(const tela::DesktopPlacement& placement) {
    placed_ = "Settled at " + std::to_string(placement.x) + ", " + std::to_string(placement.y) + " ("
        + tela::desktop_corner_name(placement.corner) + " +" + std::to_string(std::lround(placement.margin_x))
        + " +" + std::to_string(std::lround(placement.margin_y)) + ")";
}

void DesktopProbeContent::refresh(tela::Runtime& runtime, const std::string& hovered) {
    const Shown shown{clicks_, placed_, hovered};
    if(declared_ == shown) return;
    tela::Document document;
    auto frame = at(0, 0, desktop_probe_width);
    frame.height = desktop_probe_height; frame.padding = 0;
    document.canvas("desktop-probe.backdrop", backdrop_drawing(), frame);
    document.text("desktop-probe.title", "Tela desktop overlay", at(inset, 12, 280));
    document.text("desktop-probe.hint", "Clicks pass through to the windows below, except on the button and the grip.",
                  at(inset, 44, desktop_probe_width - 2 * inset, 2));
    document.button("desktop-probe.count", "Clicked " + std::to_string(clicks_) + " times", [this] { ++clicks_; },
                    at(inset, 108, 200));
    document.text("desktop-probe.placed", placed_, at(inset, 148, desktop_probe_width - 2 * inset));
    document.text("desktop-probe.hovered", hovered.empty() ? "Pointer: elsewhere" : "Pointer: " + hovered,
                  at(inset, 176, desktop_probe_width - 2 * inset));
    auto handle = at(desktop_probe_width - inset - grip_size, 12, grip_size);
    handle.height = grip_size; handle.padding = 0;
    document.canvas(desktop_probe_grip, grip_drawing(), handle, tela::InputPolicy::exclusive);
    runtime.document(std::move(document));
    declared_ = shown;
}
