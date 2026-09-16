// @implements SPEC-TL-SOURCE-VIEW
#include "source_options.hpp"
#include "source_content.hpp"
#include <tela/windows_view.hpp>
#include <iostream>
#include <windows.h>

int wmain(int argc, wchar_t** argv) {
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    try {
        const auto options = source_options(argc, argv);
        SourceContent content(tela::load_source(options.source, options.line));
        tela::Runtime runtime;
        auto theme = runtime.theme();
        theme.font_size = static_cast<float>(options.font_size);
        theme.line_height = theme.font_size * 1.5f;
        theme.panel.a = 255;
        runtime.theme(theme);
        tela::PictorSurface renderer(options.font);
        tela::WindowsView view(runtime, renderer, "Tela Source", 1100, 800, false);
        while(view.pump()) {
            content.refresh(runtime);
            view.synchronize();
            MsgWaitForMultipleObjects(0, nullptr, FALSE, 50, QS_ALLINPUT);
        }
        return 0;
    } catch(const std::exception& error) {
        std::cerr << "Tela Source: " << error.what() << '\n';
        // A launcher may have no console. Keep input/encoding errors visible there too.
        MessageBoxA(nullptr, error.what(), "Tela Source", MB_OK | MB_ICONERROR);
        return 2;
    }
}
