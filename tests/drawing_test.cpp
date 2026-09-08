// @spec Overlay drawing
#include <tela/drawing_raster.hpp>
#include <tela/runtime.hpp>
#include <tela/input_regions.hpp>
#include <algorithm>
#include <limits>
#include <iostream>
#include <stdexcept>
namespace {
void require(bool value,const char* why) { if(!value) throw std::runtime_error(why); }
tela::PixelSurface surface() { return {20,20,std::vector<unsigned char>(20*20*4)}; }
unsigned channel(const tela::PixelSurface& s,int x,int y,int c=3) { return s.pixels[(y*s.width+x)*4+c]; }
void pixels() {
    auto out=surface(); tela::Drawing fill;
    fill.rectangle({0,0,8,8},{255,0,0,128});
    tela::paint_drawing(out,fill,{},1,{3,0,2,8});
    require(channel(out,2,3)==0&&channel(out,5,3)==0,"physical clip must bound writes");
    require(channel(out,3,3)==128&&channel(out,3,3,2)==128,"red must be premultiplied BGRA");
    tela::Drawing blue; blue.rectangle({0,0,8,8},{0,0,255,128});
    tela::paint_drawing(out,blue,{},1,{3,0,2,8});
    require(channel(out,3,3)==192&&channel(out,3,3,2)==64&&channel(out,3,3,0)==128,"source-over blend");
    out=surface(); tela::Drawing dashed; dashed.line({1,4.5f},{18,4.5f},{{255,255,255,255},1,3,3});
    tela::paint_drawing(out,dashed,{},1,{0,0,20,20});
    require(channel(out,2,4)==255&&channel(out,5,4)==0&&channel(out,8,4)==255,"dash and gap must be visible");
    out=surface(); tela::Drawing round; round.rectangle({2,2,12,12},{255,255,255,255},{{0,0,0,0},0},5);
    tela::paint_drawing(out,round,{},1,{0,0,20,20});
    require(channel(out,2,2)==0&&channel(out,8,8)==255,"rounded corner must remain transparent");
    out=surface(); tela::Drawing ellipse; ellipse.ellipse({1,1,6,6},{255,255,255,255});
    tela::paint_drawing(out,ellipse,{1,1},2,{0,0,20,20});
    require(channel(out,10,10)==255&&channel(out,4,4)==0&&channel(out,2,10)==0,"DPI and origin apply once");
}
void declarations() {
    tela::Drawing drawing; drawing.line({1,1},{10,10},{{255,255,255,255},2});
    try { drawing.line({std::numeric_limits<float>::quiet_NaN(),0},{1,1},{}); throw std::logic_error("accepted NaN"); }
    catch(const std::invalid_argument&) {}
    require(drawing.shapes().size()==1,"invalid declaration must be atomic");
    tela::Runtime runtime;runtime.viewport({"host","view",1,0,0,100,100,1,true,true});
    auto doc=[&] { tela::Document d;d.canvas("drawing",drawing,{.width=100,.height=100});return d; };
    runtime.document(doc());runtime.frame_presented();runtime.document(doc());
    require(!runtime.needs_frame(),"identical drawing must sleep");
    require(runtime.hit(5,5)==tela::InputPolicy::passthrough,"drawing must not steal input");
    drawing.ellipse({20,20,10,10},{0,255,0,255});runtime.document(doc());
    require(runtime.needs_frame(),"drawing changes must invalidate");
}
void work_budget() {
    tela::PixelSurface out{512,512,std::vector<unsigned char>(512*512*4)};
    std::vector<tela::Point> points;
    for(unsigned i=0;i<300;++i) points.push_back(i%2?tela::Point{511,511}:tela::Point{0,0});
    tela::Drawing drawing;drawing.polyline(std::move(points),{{255,255,255,255},2});
    try {tela::paint_drawing(out,drawing,{},1,{0,0,512,512});throw std::logic_error("unbounded work accepted");}
    catch(const std::length_error&) {}
    require(std::all_of(out.pixels.begin(),out.pixels.end(),[](auto v){return v==0;}),"budget rejection must not partially paint");
}
}
int main() { try { pixels();declarations();work_budget();return 0; } catch(const std::exception& e) { std::cerr<<e.what()<<'\n';return 1; } }
