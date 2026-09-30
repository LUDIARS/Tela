// @spec Overlay drawing
#include <tela/drawing_raster.hpp>
#include "shape_distance.hpp"
#include "pixel_blend.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <stdexcept>

namespace tela {
namespace {
void validate(const PixelSurface& out,Point origin,float scale,Rect clip) {
    if(out.width<0||out.height<0||out.width>16384||out.height>16384||
        static_cast<std::uint64_t>(out.width)*out.height>16777216||
        out.pixels.size()!=static_cast<std::size_t>(out.width)*out.height*4)
        throw std::invalid_argument("Invalid BGRA surface");
    for(float value:{origin.x,origin.y,scale,clip.x,clip.y,clip.width,clip.height})
        if(!std::isfinite(value)||std::abs(value)>1000000) throw std::invalid_argument("Invalid raster geometry");
    if(scale<.25f||scale>8) throw std::invalid_argument("Invalid raster DPI scale");
}
void color(unsigned char* pixel,Color value,float coverage) {
    const auto a=static_cast<unsigned char>(std::lround(value.a*std::clamp(coverage,0.f,1.f)));
    raster::blend(pixel,value.b*a/255,value.g*a/255,value.r*a/255,a);
}
unsigned char lerp(unsigned char a,unsigned char b,float t) { return static_cast<unsigned char>(std::lround(a+(b-a)*t)); }
Color fill_at(const Shape& shape,Point p) {
    if(!shape.gradient) return shape.fill;
    const auto& g=*shape.gradient;
    const float dx=g.end.x-g.start.x,dy=g.end.y-g.start.y,length=dx*dx+dy*dy;
    const float t=length==0?1:std::clamp(((p.x-g.start.x)*dx+(p.y-g.start.y)*dy)/length,0.f,1.f);
    return {lerp(g.from.r,g.to.r,t),lerp(g.from.g,g.to.g,t),lerp(g.from.b,g.to.b,t),lerp(g.from.a,g.to.a,t)};
}
// Distance from the visible edge: the stroke's outer edge, or the fill boundary without a stroke.
float glow_coverage(const Shape& shape,float distance) {
    const auto& glow=shape.stroke.glow;
    if(glow.radius<=0 || glow.color.a==0) return 0;
    const float edge=shape.stroke.width>0?std::abs(distance)-shape.stroke.width/2:distance;
    if(edge<=0 || edge>=glow.radius) return 0;
    const float falloff=1-edge/glow.radius;
    return falloff*falloff;
}
// Bilinear sample of premultiplied BGRA; texel centers sit at half-integer positions.
std::array<float,4> sample(const Image& image,float u,float v) {
    const float x=std::clamp(u*image.width-.5f,0.f,static_cast<float>(image.width-1));
    const float y=std::clamp(v*image.height-.5f,0.f,static_cast<float>(image.height-1));
    const int x0=static_cast<int>(x),y0=static_cast<int>(y);
    const int x1=std::min(x0+1,image.width-1),y1=std::min(y0+1,image.height-1);
    const float fx=x-x0,fy=y-y0;
    std::array<float,4> result{};
    for(int c=0;c<4;++c) {
        auto at=[&](int px,int py){ return static_cast<float>(image.pixels[(static_cast<std::size_t>(py)*image.width+px)*4+c]); };
        const float top=at(x0,y0)+(at(x1,y0)-at(x0,y0))*fx,bottom=at(x0,y1)+(at(x1,y1)-at(x0,y1))*fx;
        result[c]=top+(bottom-top)*fy;
    }
    return result;
}
void image_pixel(unsigned char* pixel,const Shape& shape,Point p,float coverage) {
    const auto& b=shape.bounds;
    if(b.width<=0||b.height<=0) return;
    const float k=std::clamp(coverage,0.f,1.f)*shape.opacity;
    if(k<=0) return;
    const auto texel=sample(*shape.image,(p.x-b.x)/b.width,(p.y-b.y)/b.height);
    auto channel=[&](int c){ return static_cast<unsigned char>(std::clamp(std::lround(texel[c]*k),0l,255l)); };
    raster::blend(pixel,channel(0),channel(1),channel(2),channel(3));
}
Rect pixel_extent(const Shape& shape,Point origin,float scale,Rect clip) {
    auto bounds=raster::extent(shape);
    const float padding=shape.stroke.width/2+shape.stroke.glow.radius+1/scale;
    bounds={ (origin.x+bounds.x-padding)*scale,(origin.y+bounds.y-padding)*scale,
        (bounds.width+2*padding)*scale,(bounds.height+2*padding)*scale };
    return intersect(bounds,clip);
}
void shape_pixels(PixelSurface& out,const Shape& shape,Point origin,float scale,Rect bounds) {
    const int left=static_cast<int>(std::ceil(bounds.x-.5f)),top=static_cast<int>(std::ceil(bounds.y-.5f));
    const int right=static_cast<int>(std::ceil(bounds.x+bounds.width-.5f));
    const int bottom=static_cast<int>(std::ceil(bounds.y+bounds.height-.5f));
    for(int y=top;y<bottom;++y) for(int x=left;x<right;++x) {
        const Point point{(x+.5f)/scale-origin.x,(y+.5f)/scale-origin.y};
        const float distance=raster::distance(shape,point);
        auto* pixel=&out.pixels[(static_cast<std::size_t>(y)*out.width+x)*4];
        if(shape.kind==ShapeKind::image) { image_pixel(pixel,shape,point,.5f-distance*scale); continue; }
        const float opacity=shape.opacity;
        if(const float glow=glow_coverage(shape,distance);glow>0) color(pixel,shape.stroke.glow.color,glow*opacity);
        if(shape.kind!=ShapeKind::polyline) color(pixel,fill_at(shape,point),(.5f-distance*scale)*opacity);
        if(shape.stroke.width>0) color(pixel,shape.stroke.color,
            (.5f-(std::abs(distance)-shape.stroke.width/2)*scale)*opacity);
    }
}
}
void paint_drawing(PixelSurface& out,const Drawing& drawing,Point origin,float scale,Rect clip) {
    validate(out,origin,scale,clip);
    clip=intersect(clip,{0,0,static_cast<float>(out.width),static_cast<float>(out.height)});
    std::uint64_t work=0;
    // Validate the complete work budget before changing the destination.
    for(const auto& shape:drawing.shapes()) {
        const auto b=pixel_extent(shape,origin,scale,clip);
        work+=static_cast<std::uint64_t>(std::ceil(b.width)+1)*(static_cast<std::uint64_t>(std::ceil(b.height))+1)*
            std::max(std::size_t{1},shape.points.size());
        if(work>64000000) throw std::length_error("CPU drawing work budget exceeded");
    }
    for(const auto& shape:drawing.shapes()) {
        const auto bounds=pixel_extent(shape,origin,scale,clip);
        if(bounds.width>0&&bounds.height>0) shape_pixels(out,shape,origin,scale,bounds);
    }
}
}
