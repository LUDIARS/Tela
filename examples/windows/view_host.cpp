// @implements SPEC-TL-VIEW-HOST
// @spec View host
#include "view_options.hpp"
#include "spec_view_content.hpp"
#include <tela/windows_view.hpp>
#include <chrono>
#include <iostream>
#include <windows.h>

namespace {
using Clock = std::chrono::steady_clock;

int run(const ViewOptions& config) {
    tela::Runtime runtime;
    if(config.fontSize) {
        auto theme = runtime.theme();
        const float size = static_cast<float>(config.fontSize);
        // The default theme pairs 16 px text with a 24 px line; keep that ratio at any size.
        theme.line_height = size * (theme.font_size > 0 ? theme.line_height / theme.font_size : 1.5f);
        theme.font_size = size;
        runtime.theme(theme);
    }
    tela::PictorSurface renderer(config.font);
    SpecViewContent content(config.specView);
    const auto natural = content.natural_bounds();
    // The exported size is the window's default so the content is read at 1:1.
    const int width = config.width ? config.width : static_cast<int>(natural.width);
    const int height = config.height ? config.height : static_cast<int>(natural.height);
    tela::WindowsView view(runtime, renderer, config.title, width, height, config.fullscreen);
    const auto started = Clock::now();
    while(view.pump()) {
        if(config.seconds && Clock::now() - started >= std::chrono::seconds(config.seconds)) break;
        content.refresh(runtime);
        view.synchronize();
        // The view owns its messages; wait for one instead of spinning.
        MsgWaitForMultipleObjects(0, nullptr, FALSE, 50, QS_ALLINPUT);
    }
    return 0;
}
}

int main(int argc, char** argv) {
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    try {
        return run(viewOptions(argc, argv));
    } catch(const std::exception& error) {
        std::cerr << "Tela view: " << error.what() << '\n';
        return 2;
    }
}
