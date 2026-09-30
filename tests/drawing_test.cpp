// @spec Overlay drawing
#include <tela/drawing_raster.hpp>
#include <tela/runtime.hpp>
#include <tela/input_regions.hpp>
#include <algorithm>
#include <limits>
#include <memory>
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
void effects() {
    // Gradient: clamps to the end colours and blends linearly between them.
    auto out=surface(); tela::Drawing gradient;
    gradient.gradient_rectangle({0,0,20,4},tela::LinearGradient{{5,0},{15,0},{255,0,0,255},{0,0,255,255}});
    tela::paint_drawing(out,gradient,{},1,{0,0,20,20});
    require(channel(out,1,1,2)==255&&channel(out,1,1,0)==0,"before the start the start colour holds");
    require(channel(out,18,1,0)==255&&channel(out,18,1,2)==0,"after the end the end colour holds");
    require(channel(out,10,1,2)>100&&channel(out,10,1,2)<155&&channel(out,10,1,0)>100,"the middle mixes both colours");
    // Glow: soft light outside the stroke edge that fades to nothing at the radius.
    out=surface(); tela::Drawing glow;
    glow.line({2,10.5f},{18,10.5f},{{255,255,255,255},2,0,0,{{0,255,0,255},4}});
    tela::paint_drawing(out,glow,{},1,{0,0,20,20});
    require(channel(out,10,10)==255,"the stroke itself stays opaque");
    require(channel(out,10,12,1)>0&&channel(out,10,12,1)<255,"just outside the stroke glows green");
    require(channel(out,10,13)>channel(out,10,14),"the glow fades with distance");
    require(channel(out,10,16)==0,"beyond the radius nothing is painted");
    // Image: bilinear, premultiplied, opacity-scaled, positioned by the declared rectangle.
    auto image=std::make_shared<tela::Image>(tela::Image{2,1,{0,0,255,255, 255,0,0,255}});
    out=surface(); tela::Drawing picture; picture.image({4,4,8,8},image,.5f);
    tela::paint_drawing(out,picture,{},1,{0,0,20,20});
    require(channel(out,2,6)==0,"outside the rectangle stays transparent");
    require(channel(out,5,6,2)>100&&channel(out,5,6)>=127&&channel(out,5,6)<=128,"left texel is red at half opacity");
    require(channel(out,10,6,0)>100&&channel(out,10,6,2)<30,"right texel is blue");
    // Opacity and invalid declarations.
    tela::Drawing invalid;
    for(auto attempt:{+[](tela::Drawing& d){ d.line({0,0},{1,1},{{255,255,255,255},1,0,0,{{0,0,0,255},200}}); },
                      +[](tela::Drawing& d){ d.image({0,0,1,1},nullptr); },
                      +[](tela::Drawing& d){ d.image({0,0,1,1},std::make_shared<tela::Image>(tela::Image{2,2,{0}})); },
                      +[](tela::Drawing& d){ d.image({0,0,1,1},std::make_shared<tela::Image>(tela::Image{1,1,{0,0,0,0}}),2); }}) {
        try { attempt(invalid); throw std::logic_error("invalid effect accepted"); } catch(const std::invalid_argument&) {}
    }
    require(invalid.shapes().empty(),"rejected effects leave the drawing unchanged");
    // Identity: the same image object is equal, a copy with the same pixels is a new declaration.
    tela::Drawing a,b,c; a.image({0,0,2,2},image); b.image({0,0,2,2},image);
    c.image({0,0,2,2},std::make_shared<tela::Image>(*image));
    require(a==b&&!(a==c),"image identity decides redraw");
}
void trimming() {
    const std::vector<tela::Point> path{{0,0},{10,0},{10,10}};
    require(tela::trim_polyline(path,0).empty(),"zero progress draws nothing");
    require(tela::trim_polyline(path,1)==path,"full progress keeps the path");
    const auto half=tela::trim_polyline(path,.5f);
    require(half.size()==2&&half.back()==tela::Point{10,0},"half of 20 ends at the corner");
    const auto three=tela::trim_polyline(path,.75f);
    require(three.size()==3&&three.back()==tela::Point{10,5},"trimming cuts inside a segment");
    require(tela::trim_polyline({{1,1}},1).empty(),"a single point is not a line");
    try { tela::trim_polyline(path,std::numeric_limits<float>::quiet_NaN()); throw std::logic_error("NaN accepted"); }
    catch(const std::invalid_argument&) {}
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
int main() { try { pixels();declarations();effects();trimming();work_budget();return 0; } catch(const std::exception& e) { std::cerr<<e.what()<<'\n';return 1; } }
