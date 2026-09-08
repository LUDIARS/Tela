// @spec Overlay drawing
#include "../examples/windows/probe_content.hpp"
#include <algorithm>
#include <stdexcept>
#include <iostream>
namespace {
void require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
const tela::PlacedElement& button(const tela::Runtime& r){
    const auto found=std::find_if(r.elements().begin(),r.elements().end(),[](const auto& e){return e.element.id=="exclusive";});
    if(found==r.elements().end())throw std::runtime_error("Missing probe button");
    return *found;
}
}
int main(){try{
    tela::Runtime r;r.viewport({"probe","client",1,0,0,850,550,1,true,true});
    ProbeContent content;int clicks=0;content.refresh(r,clicks);r.frame_presented();
    content.refresh(r,clicks);require(!r.needs_frame(),"idle coordinator must not invalidate");
    const auto bounds=button(r).bounds;
    tela::HostPointerEvent e;e.sequence=1;e.gesture_id=1;e.viewport_revision=1;
    e.desktop_x=static_cast<int>(bounds.x+10);e.desktop_y=static_cast<int>(bounds.y+10);
    e.button=tela::PointerButton::primary;e.phase=tela::PointerPhase::down;r.pointer(e,tela::InputSource::native);
    e.sequence=2;e.phase=tela::PointerPhase::up;r.pointer(e,tela::InputSource::native);
    r.pointer(e,tela::InputSource::native);require(clicks==1,"one physical gesture must increment once");
    content.refresh(r,clicks);require(button(r).element.label=="Tela button: 1","coordinator must publish changed state");
    require(r.state("exclusive")->activations==1,"rebuild must preserve action state");
    r.frame_presented();content.refresh(r,clicks);require(!r.needs_frame(),"unchanged state must sleep");
    auto view=r.viewport();++view.revision;view.width=600;r.viewport(view);content.refresh(r,clicks);
    require(r.needs_frame(),"viewport change must refresh annotation placement");
    view.visible=false;++view.revision;r.viewport(view);content.refresh(r,clicks);
    require(!r.needs_frame()&&r.state("probe.target-name.label")==nullptr,"hidden target must remove annotation and stop rendering");
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
