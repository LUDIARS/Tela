// @spec Overlay drawing
#include <tela/callout.hpp>
#include <tela/runtime.hpp>
#include <iostream>
#include <stdexcept>
namespace {
void require(bool value,const char* why) { if(!value) throw std::runtime_error(why); }
void placement() {
    const tela::Rect area{0,0,500,400};
    for(auto anchor:{tela::Point{5,5},{495,5},{5,395},{495,395}}) {
        tela::Callout c;c.anchor=anchor;const auto p=tela::place_callout(c,area);
        require(p.has_value(),"visible corner anchor must produce a callout");
        require(p->label.x>=12&&p->label.y>=12&&p->label.x+p->label.width<=488&&p->label.y+p->label.height<=388,"label must stay inside margin");
        require(p->attachment.x>=p->label.x-.01f&&p->attachment.x<=p->label.x+p->label.width+.01f,"line attaches to label bounds");
    }
    tela::Callout c;c.anchor={200,200};c.visible=false;
    require(!tela::place_callout(c,area),"hidden target must not produce a label");
    c.visible=true;c.anchor={501,200};require(!tela::place_callout(c,area),"offscreen target must hide");
    c.anchor={20,20};require(!tela::place_callout(c,{0,0,50,50}),"small viewport must hide rather than invert clamp bounds");
}
void declaration() {
    tela::Callout c;c.anchor={200,200};const tela::Rect area{0,0,500,400};
    tela::Document d;tela::callout(d,"target","KENZOKU",c,area);
    require(d.elements().size()==2,"callout composes a drawing and text");
    tela::Runtime r;r.viewport({"host","view",1,0,0,500,400,1,true,true});r.document(d);r.frame_presented();
    tela::Document same;tela::callout(same,"target","KENZOKU",c,area);r.document(same);
    require(!r.needs_frame(),"identical callout must not schedule a frame");
    c.anchor.x+=10;tela::Document moved;tela::callout(moved,"target","KENZOKU",c,area);r.document(moved);
    require(r.needs_frame(),"anchor movement must update line and label");
    tela::Document conflict;conflict.text("target.label","existing");
    try { tela::callout(conflict,"target","replacement",c,area);throw std::logic_error("duplicate accepted"); }
    catch(const std::invalid_argument&) {}
    require(conflict.elements().size()==1,"failed composite declaration must preserve document");
    c.visible=false;tela::Document hidden;tela::callout(hidden,"target","KENZOKU",c,area);r.document(hidden);
    require(r.elements().empty(),"target loss must remove label and line");
}
}
int main() { try {placement();declaration();return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;} }
