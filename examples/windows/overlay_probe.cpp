#include "overlay_probe.hpp"
#include <stdexcept>
// @spec Overlay lifecycle
void synchronizeProbe(tela::Runtime& runtime,HWND target){
    RECT rect{};POINT pos{};if(!IsWindow(target)){runtime.disconnect();return;}
    if(!GetClientRect(target,&rect)||!ClientToScreen(target,&pos))throw std::runtime_error("Cannot read target geometry");
    auto next=runtime.viewport();
    const float dpi=GetDpiForWindow(target)/96.f;
    const bool visible=IsWindowVisible(target)&&!IsIconic(target);
    const bool focused=GetAncestor(GetForegroundWindow(),GA_ROOT)==target;
    if(next.host_id=="probe"&&next.desktop_x==pos.x&&next.desktop_y==pos.y&&next.width==rect.right&&next.height==rect.bottom&&next.dpi_scale==dpi&&next.visible==visible&&next.focused==focused)return;
    ++next.revision;next.host_id="probe";next.view_id="client";next.desktop_x=pos.x;next.desktop_y=pos.y;
    next.width=rect.right;next.height=rect.bottom;next.dpi_scale=dpi;next.visible=visible;next.focused=focused;runtime.viewport(next);
}
tela::Document probeDocument(int& clicks,tela::Runtime& runtime){
    tela::Document d;
    d.panel("probe",[&]{
        d.text("title","Tela / Pictor transparent overlay");
        d.text("passthrough","Click this text: the host receives input");
        d.button("exclusive","Tela button: "+std::to_string(clicks),[&clicks,&runtime]{++clicks;runtime.document(probeDocument(clicks,runtime));});
        d.text("hint","Move / minimize the host; press Esc to close");
    },{.width=420,.positioned=true,.x=30,.y=30});
    return d;
}
