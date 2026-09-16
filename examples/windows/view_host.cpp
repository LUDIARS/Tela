// @implements SPEC-TL-VIEW-HOST
// @spec View host
#include "view_options.hpp"
#include "graph_content.hpp"
#include "spec_view_content.hpp"
#include <tela/windows_view.hpp>
#include <chrono>
#include <iostream>
#include <memory>
#include <windows.h>

namespace {
using Clock = std::chrono::steady_clock;

void applyTheme(tela::Runtime& runtime, int size) {
    if(!size) return;
    auto theme = runtime.theme();
    const float wanted = static_cast<float>(size);
    // The default theme pairs 16 px text with a 24 px line; keep that ratio at any size.
    theme.line_height = wanted * (theme.font_size > 0 ? theme.line_height / theme.font_size : 1.5f);
    theme.font_size = wanted;
    runtime.theme(theme);
}

int run(const ViewOptions& config) {
    tela::Runtime runtime;
    applyTheme(runtime, config.fontSize);
    tela::PictorSurface renderer(config.font);
    // One content source per run; the options already rejected having both or neither.
    std::unique_ptr<SpecViewContent> specs;
    std::unique_ptr<GraphContent> graph;
    tela::Rect natural{};
    if(!config.specView.empty()) {
        specs = std::make_unique<SpecViewContent>(config.specView);
        natural = specs->natural_bounds();
    } else {
        graph = std::make_unique<GraphContent>(config.graph);
        natural = graph->natural_bounds();
    }
    // The exported size is the window's default so the content is read at 1:1.
    const int width = config.width ? config.width : static_cast<int>(natural.width);
    const int height = config.height ? config.height : static_cast<int>(natural.height);
    tela::WindowsView view(runtime, renderer, config.title, width, height, config.fullscreen);
    const auto started = Clock::now();
    while(view.pump()) {
        if(config.seconds && Clock::now() - started >= std::chrono::seconds(config.seconds)) break;
        if(specs) specs->refresh(runtime); else graph->refresh(runtime);
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
