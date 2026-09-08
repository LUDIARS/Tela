// @spec Overlay drawing
#include "probe_content.hpp"
#include "overlay_probe.hpp"
bool ProbeContent::refresh_from_target(tela::Runtime& runtime,std::uintptr_t target,int& clicks) {
    const auto window=reinterpret_cast<HWND>(target);
    synchronizeProbe(runtime,window);
    refresh(runtime,clicks);
    return IsWindow(window)!=FALSE;
}
void ProbeContent::refresh(tela::Runtime& runtime,int& clicks) {
    const auto& view=runtime.viewport();
    const Revision revision{view.width,view.height,view.dpi_scale,view.visible,clicks};
    if(declared_==revision) return;
    runtime.document(probeDocument(clicks,runtime));
    declared_=revision;
}
