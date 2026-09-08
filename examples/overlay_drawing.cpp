// @spec Overlay drawing
#include <tela/callout.hpp>
#include <tela/runtime.hpp>

int main() {
    tela::Document ui;
    tela::Drawing decoration;
    decoration.rectangle({10,10,280,100},{25,30,42,220},{{90,160,240,255},2},12);
    decoration.ellipse({24,28,24,24},{80,220,140,255});
    decoration.polyline({{60,90},{100,55},{140,70},{200,30}},{{240,180,70,255},2});
    ui.canvas("status.decoration",decoration,{.width=320,.height=140});
    ui.text("status.caption","Tracked window",{.positioned=true,.x=60,.y=20});
    tela::Callout name; name.anchor={440,320};name.leader.dash=6;name.leader.gap=4;
    tela::callout(ui,"creature.name","KENZOKU",name,{0,0,800,600});
    tela::Runtime runtime;runtime.viewport({"example","overlay",1,0,0,800,600,1,true,true});
    runtime.document(std::move(ui));
    // The host presents with PictorSurface/WindowsOverlay, or another adapter.
    // This standalone example only constructs the portable declaration.
    return runtime.elements().size()==4?0:1;
}
