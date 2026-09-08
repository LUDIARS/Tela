#include "overlay_options.hpp"
#include "overlay_session.hpp"
#include "overlay_diagnostics.hpp"
#include <windows.h>
#include <exception>
#include <iostream>
// @spec Overlay lifecycle
int main(int argc,char** argv) {
    try {
        const auto config=options(argc,argv);
        SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
        const OverlayDiagnostics diagnostics;
        const auto result=runOverlay(config);
        diagnostics.write(config.report,result);
        return 0;
    } catch(const std::exception& e) {
        std::cerr<<"Tela: "<<e.what()<<'\n'; return 1;
    }
}
