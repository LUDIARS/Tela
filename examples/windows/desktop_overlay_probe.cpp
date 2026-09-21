// @implements SPEC-TL-DESKTOP-OVERLAY
// @spec Desktop overlay
// Floats the fixed probe declaration on the desktop so a person can check by eye that it is
// translucent, that the windows below keep their input, and that the grip moves it.
#include "desktop_probe_content.hpp"
#include "desktop_probe_options.hpp"
#include <tela/windows_desktop_overlay.hpp>
#include <chrono>
#include <iostream>
#include <windows.h>

namespace {
using Clock = std::chrono::steady_clock;
constexpr DWORD idle_wait_ms = 100;

// USER and GDI objects the process holds, compared before the surface exists and after it is gone.
struct Objects { DWORD gdi, user; };
Objects objects() {
    return {GetGuiResources(GetCurrentProcess(), GR_GDIOBJECTS), GetGuiResources(GetCurrentProcess(), GR_USEROBJECTS)};
}

void run(const DesktopProbeOptions& options) {
    tela::Runtime runtime;
    tela::PictorSurface renderer(options.font);
    DesktopProbeContent content;
    tela::DesktopOverlayOptions surface;
    surface.placement.width = desktop_probe_width;
    surface.placement.height = desktop_probe_height;
    surface.placement.corner = options.corner;
    surface.grip = desktop_probe_grip;
    surface.moved = [&content](const tela::DesktopPlacement& placement) {
        content.moved(placement);
        // A real caller stores this; the probe prints it so the spot can be checked by hand.
        std::cout << "moved " << placement.x << ' ' << placement.y << ' ' << tela::desktop_corner_name(placement.corner)
                  << ' ' << placement.margin_x << ' ' << placement.margin_y << std::endl;
    };
    tela::WindowsDesktopOverlay overlay(runtime, renderer, std::move(surface));
    const auto started = Clock::now();
    while(Clock::now() - started < std::chrono::seconds(options.seconds)) {
        MSG message{};
        while(PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
        content.refresh(runtime, overlay.hovered());
        overlay.synchronize();
        MsgWaitForMultipleObjects(0, nullptr, FALSE, idle_wait_ms, QS_ALLINPUT);
    }
}
}

int main(int argc, char** argv) {
    try {
        const auto options = desktopProbeOptions(argc, argv);
        const auto before = objects();
        run(options);
        const auto after = objects();
        std::cout << "gdi " << before.gdi << " -> " << after.gdi << ", user " << before.user << " -> " << after.user << '\n';
        return 0;
    } catch(const std::exception& error) {
        std::cerr << "Tela desktop overlay probe: " << error.what() << '\n';
        return 2;
    }
}
