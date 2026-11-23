// @implements SPEC-TL-CONTROLS
// @spec Controls
// Native preview of the control gallery: tela_controls --font <file.ttf> [--seconds N]
#include "../controls/controls_gallery.hpp"
#include <tela/windows_view.hpp>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <string>
#include <windows.h>

namespace {
struct Options { std::string font; int seconds{}; };
Options parse(int argc, char** argv) {
    Options options;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (i + 1 >= argc) throw std::invalid_argument("missing value for " + arg);
        if (arg == "--font") options.font = argv[++i];
        else if (arg == "--seconds") options.seconds = std::stoi(argv[++i]);
        else throw std::invalid_argument("unknown option " + arg);
    }
    if (options.font.empty()) throw std::invalid_argument("usage: tela_controls --font <file.ttf> [--seconds N]");
    if (options.seconds < 0 || options.seconds > 86400) throw std::invalid_argument("--seconds must be 0..86400");
    return options;
}
}

int main(int argc, char** argv) {
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    try {
        const auto options = parse(argc, argv);
        tela::Runtime runtime;
        tela::PictorSurface renderer(options.font);
        tela::WindowsView view(runtime, renderer, "Tela controls", 520, 720, false);
        ControlsGallery gallery(runtime);
        const auto started = std::chrono::steady_clock::now();
        while (view.pump()) {
            if (options.seconds && std::chrono::steady_clock::now() - started >= std::chrono::seconds(options.seconds)) break;
            view.synchronize();
            MsgWaitForMultipleObjects(0, nullptr, FALSE, 50, QS_ALLINPUT);
        }
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Tela controls: " << error.what() << '\n';
        return 2;
    }
}
