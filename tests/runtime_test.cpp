// @implements SPEC-TL-RUNTIME
// @spec Runtime
// @implements SPEC-TL-INPUT
// @spec Input ownership
#include <tela/runtime.hpp>
#include <tela/input_regions.hpp>
#include <algorithm>
#include <functional>
#include <stdexcept>
#include <iostream>

namespace {
void require(bool condition,const char* message){if(!condition)throw std::runtime_error(message);}
tela::HostPointerEvent event(std::uint64_t sequence,tela::PointerPhase phase,int x=20,int y=20){
    tela::HostPointerEvent e;e.sequence=sequence;e.viewport_revision=1;e.gesture_id=7;e.phase=phase;e.button=tela::PointerButton::primary;e.desktop_x=x;e.desktop_y=y;return e;
}
}
int main(){try{
    tela::Runtime r;r.viewport({"host","view",1,0,0,300,200,1,true,true});
    int old=0,current=0;
    tela::Document first;first.button("stable","Old",[&]{++old;},{.width=100});r.document(std::move(first));
    r.frame_presented();require(!r.needs_frame(),"unchanged runtime must sleep");
    r.pointer(event(1,tela::PointerPhase::down),tela::InputSource::native);
    tela::Document next;next.button("stable","New label",[&]{++current;},{.width=100});r.document(std::move(next));
    require(r.state("stable")->pressed,"stable ID must retain press through redeclaration");
    r.pointer(event(2,tela::PointerPhase::up),tela::InputSource::native);
    r.pointer(event(2,tela::PointerPhase::up),tela::InputSource::native);
    require(old==0&&current==1,"dispatch current callback once");
    require(r.state("stable")->activations==1,"activation state survives");
    r.pointer(event(3,tela::PointerPhase::down),tela::InputSource::native);
    r.pointer(event(4,tela::PointerPhase::up,200,100),tela::InputSource::native);
    require(current==1&&!r.captured(),"drag out must cancel click");
    r.pointer(event(5,tela::PointerPhase::down),tela::InputSource::native);
    auto hidden=r.viewport();hidden.visible=false;r.viewport(hidden);
    require(!r.captured()&&!r.needs_frame(),"hidden overlay releases capture and sleeps");
    hidden.visible=true;r.viewport(hidden);require(r.needs_frame(),"show invalidates previous frame");
    tela::Document shared;shared.button("shared","Observe",[&]{++current;},{.width=100},tela::InputPolicy::shared);r.document(shared);
    r.pointer(event(6,tela::PointerPhase::down),tela::InputSource::native);
    require(!r.captured(),"native channel must not own shared input");
    r.pointer(event(1,tela::PointerPhase::down),tela::InputSource::host_observation);
    auto stale=event(2,tela::PointerPhase::up);stale.viewport_revision=0;r.pointer(stale,tela::InputSource::host_observation);
    require(r.captured(),"stale geometry cannot complete gesture");
    r.pointer(event(2,tela::PointerPhase::up),tela::InputSource::host_observation);require(current==2,"shared gesture dispatch once");
    r.pointer(event(3,tela::PointerPhase::down),tela::InputSource::host_observation);r.disconnect();require(!r.captured()&&!r.needs_frame(),"disconnect cancels");
    tela::Document layout;layout.panel("p",[&]{layout.button("b","Clipped",{},{.width=300,.height=200});},{.width=100,.height=80});
    const auto placed=tela::arrange(layout,500,500,{});require(placed.at(1).clip.width==84&&placed.at(1).clip.height==64,"parent clipping must apply to input and paint");
    tela::Document overlap;overlap.button("lower","exclusive",{},{.width=100,.height=100,.positioned=true});
    overlap.button("upper","shared",{},{.width=50,.height=100,.positioned=true,.x=25},tela::InputPolicy::shared);
    auto regions=tela::exclusive_regions(tela::arrange(overlap,200,200,{}));require(regions.size()==1&&regions[0].fragments.size()==2,"shared region must cut through underlying exclusive window");
    for(auto fragment:regions[0].fragments)require(!fragment.contains(50,20),"shared physical click must reach host");
    // The identical-declaration fast path rebinds callbacks without relaying out.
    // Each button must keep its own action across nested panels.
    tela::Runtime rebind;rebind.viewport({"host","view",1,0,0,400,400,1,true,true});
    int a=0,b=0;
    auto build=[&](int& first,int& second){
        tela::Document doc;
        doc.panel("pa",[&]{doc.button("ba","A",[&first]{++first;},{.width=100,.height=40});},{.width=200,.height=80,.positioned=true});
        doc.panel("pb",[&]{doc.button("bb","B",[&second]{++second;},{.width=100,.height=40});},{.width=200,.height=80,.positioned=true,.y=120});
        return doc;
    };
    rebind.document(build(a,b));
    int a2=0,b2=0;rebind.document(build(a2,b2)); // same shape: takes the fast path
    const auto* placedA=&rebind.elements();
    require(placedA->size()==4,"nested declaration must place every element");
    // Click button B; only B's newly bound callback may run.
    auto press=[&](std::uint64_t seq,tela::PointerPhase p,int x,int y){
        tela::HostPointerEvent e;e.sequence=seq;e.viewport_revision=1;e.gesture_id=9;e.phase=p;
        e.button=tela::PointerButton::primary;e.desktop_x=x;e.desktop_y=y;return e;
    };
    const auto& bb=*std::find_if(placedA->begin(),placedA->end(),[](const auto& p){return p.element.id=="bb";});
    const int bx=static_cast<int>(bb.bounds.x+2),by=static_cast<int>(bb.bounds.y+2);
    rebind.pointer(press(1,tela::PointerPhase::down,bx,by),tela::InputSource::native);
    rebind.pointer(press(2,tela::PointerPhase::up,bx,by),tela::InputSource::native);
    require(b2==1&&a2==0&&a==0&&b==0,"fast-path rebind must bind each action to its own element");
    // An action may redeclare the document from inside its own dispatch. The
    // relayout that follows reassigns placed_, so the runtime must not still be
    // reading the std::function it is executing. This is the documented probe
    // interaction in spec/windows-composition.md step 3.
    tela::Runtime reentrant;reentrant.viewport({"host","view",1,0,0,300,200,1,true,true});
    int presses=0;
    std::function<void()> declare=[&]{
        tela::Document doc;
        // The changing label defeats the identical-declaration fast path and
        // forces a full relayout from inside the callback.
        doc.button("self","Clicks: "+std::to_string(presses),[&]{++presses;declare();},{.width=200,.height=40});
        reentrant.document(std::move(doc));
    };
    declare();
    reentrant.pointer(event(1,tela::PointerPhase::down),tela::InputSource::native);
    reentrant.pointer(event(2,tela::PointerPhase::up),tela::InputSource::native);
    require(presses==1,"self-redeclaring action must dispatch exactly once");
    reentrant.pointer(event(3,tela::PointerPhase::down),tela::InputSource::native);
    reentrant.pointer(event(4,tela::PointerPhase::up),tela::InputSource::native);
    require(presses==2,"runtime stays usable after a re-entrant redeclaration");
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
