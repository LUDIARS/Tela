// @spec Unity bridge
// @spec Input ownership
#include <tela/bridge.hpp>
#include <iostream>
#include <stdexcept>
namespace { void require(bool b,const char* s){if(!b)throw std::runtime_error(s);} }
int main(){try{
    tela::Runtime runtime; tela::BridgeSession session(runtime);int clicks=0;
    tela::Document doc;doc.button("shared","Shared",[&]{++clicks;},{.width=100,.height=40},tela::InputPolicy::shared);
    runtime.document(std::move(doc));
    for(unsigned generation=1;generation<=64;++generation){
        tela::BridgeMessage m;m.kind=tela::BridgeKind::hello;m.host="Unity";m.view="Scene";
        m.host_window=123;m.generation=generation;m.sequence=1;
        require(session.accept(m),"reconnect hello");
        m.kind=tela::BridgeKind::viewport;m.sequence=2;m.revision=1;
        m.viewport={"Unity","Scene",1,0,0,300,200,1,true,true};require(session.accept(m),"restored viewport");
        require(runtime.needs_frame(),"restore invalidates");runtime.frame_presented();
        m.kind=tela::BridgeKind::pointer;m.sequence=3;
        m.pointer={3,1,7,tela::PointerPhase::down,tela::PointerButton::primary,20,20};
        require(session.accept(m)&&runtime.captured(),"shared gesture started");
        auto stale=m;stale.pointer.phase=tela::PointerPhase::up;stale.pointer.sequence=4;stale.sequence=4;
        session.disconnect();require(!runtime.captured()&&!runtime.state("shared")->pressed,"disconnect releases gesture");
        require(!runtime.needs_frame(),"disconnected is hidden");
        require(!session.accept(stale)&&clicks==0,"old up rejected");
        m.kind=tela::BridgeKind::hello;m.sequence=1;m.generation=1000+generation;
        require(session.accept(m),"next generation");require(!session.accept(stale),"previous generation stays rejected");
        session.disconnect();session.disconnect();
    }
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
