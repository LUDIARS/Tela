#include "overlay_probe.hpp"
#include "probe_controls.hpp"
#include <tela/callout.hpp>
// @spec Overlay drawing
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
    declare_probe_controls(d,clicks);
    const auto& view=runtime.viewport();
    const float width=view.width/view.dpi_scale,height=view.height/view.dpi_scale;
    tela::Callout name;
    name.anchor={width*.7f,height*.65f};name.visible=view.visible;
    name.leader.dash=6;name.leader.gap=4;
    tela::callout(d,"probe.target-name","Tracked target",name,{0,0,width,height});
    return d;
}
