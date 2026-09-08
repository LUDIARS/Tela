// @spec Overlay drawing
#include "probe_controls.hpp"
void declare_probe_controls(tela::Document& d,int& clicks) {
    d.panel("probe",[&]{
        d.text("title","Tela / Pictor transparent overlay");
        d.text("passthrough","Click this text: the host receives input");
        d.button("exclusive","Tela button: "+std::to_string(clicks),[&clicks]{++clicks;});
        d.text("hint","Move / minimize the host; press Esc to close");
    },{.width=420,.positioned=true,.x=30,.y=30});
}
