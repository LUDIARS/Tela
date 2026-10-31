// @implements SPEC-TL-LIST-VIEW
// @spec List view
// Floats a `TELA_LIST_VIEW 1` file on the desktop and redraws it whenever the writer replaces
// the file. The surface lets input through except on the grip, which moves it.
#include "../desktop/list_file_source.hpp"
#include "../desktop/list_overlay_options.hpp"
#include <tela/list_view.hpp>
#include <tela/pictor_surface.hpp>
#include <tela/runtime.hpp>
#include <tela/windows_desktop_overlay.hpp>
#include <chrono>
#include <iostream>
#include <windows.h>

namespace {
using Clock = std::chrono::steady_clock;
constexpr DWORD idle_wait_ms = 250;
constexpr auto file_check_interval = std::chrono::seconds(1);
constexpr char grip_id[] = "list.grip";

void apply_font_size(tela::Runtime& runtime, float wanted) {
    auto theme = runtime.theme();
    // Keep the default theme's text-to-line ratio at any size, as the view host does.
    theme.line_height = wanted * (theme.font_size > 0 ? theme.line_height / theme.font_size : 1.5f);
    theme.font_size = wanted;
    runtime.theme(theme);
}

void run(const ListOverlayOptions& options) {
    tela::Runtime runtime;
    if(options.font_size) apply_font_size(runtime, *options.font_size);
    tela::PictorSurface renderer(options.font);
    const tela::ListViewFrame frame{options.width, options.rows, grip_id};
    tela::DesktopOverlayOptions surface;
    surface.placement.width = frame.width;
    surface.placement.height = tela::list_view_height(frame, runtime.theme());
    surface.placement.corner = options.corner;
    surface.grip = grip_id;
    surface.exclude_from_capture = options.exclude_from_capture;
    surface.moved = [](const tela::DesktopPlacement& placement) {
        // Placement is not stored by the host; the line lets a caller pass it back as --corner.
        std::cout << "moved " << placement.x << ' ' << placement.y << ' ' << tela::desktop_corner_name(placement.corner)
                  << ' ' << placement.margin_x << ' ' << placement.margin_y << std::endl;
    };
    tela::WindowsDesktopOverlay overlay(runtime, renderer, std::move(surface));
    ListFileSource source(options.file, file_check_interval);
    runtime.document(tela::list_view_document(source.current_view(), frame, runtime.theme()));
    const auto started = Clock::now();
    while(options.seconds == 0 || Clock::now() - started < std::chrono::seconds(options.seconds)) {
        MSG message{};
        while(PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) {
            if(message.message == WM_QUIT) return;
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
        if(source.poll(Clock::now())) runtime.document(tela::list_view_document(source.current_view(), frame, runtime.theme()));
        overlay.synchronize();
        MsgWaitForMultipleObjects(0, nullptr, FALSE, idle_wait_ms, QS_ALLINPUT);
    }
}
}

int main(int argc, char** argv) {
    try {
        run(listOverlayOptions(argc, argv));
        return 0;
    } catch(const std::exception& error) {
        std::cerr << "Tela list overlay: " << error.what() << '\n';
        return 2;
    }
}
