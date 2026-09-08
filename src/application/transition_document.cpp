// @spec SPEC-TL-TRANSITIONS
#include <tela/transitions.hpp>

namespace tela {
Document transition_document(const Transitions& data,const std::vector<Anchor>& anchors,const Viewport& view,
    std::function<void(const Transition&)> edit,std::function<void(const Anchor&)> add) {
    Document d;
    for(const auto& anchor:anchors) {
        if(!anchor.visible)continue;
        Layout panel;panel.width=320;panel.positioned=true;
        panel.x=(anchor.desktop_x-view.desktop_x)/view.dpi_scale+12;
        panel.y=(anchor.desktop_y-view.desktop_y)/view.dpi_scale;
        const std::string prefix="anchor/"+anchor.object_id;
        d.panel(prefix,[&]{
            d.text(prefix+"/label",anchor.label);
            for(const auto& t:data.entries()) {
                if(t.object_id!=anchor.object_id)continue;
                d.button("transition/"+t.id,t.source+" -> "+t.destination,[edit,t]{edit(t);});
                d.text("condition/"+t.id,t.condition.empty()?"Always":t.condition);
            }
            d.button(prefix+"/add","Add transition",[add,anchor]{add(anchor);});
        },panel);
    }
    if(anchors.empty())d.text("empty","Tela: select a Scene object to edit transitions",{.width=460});
    return d;
}
}
