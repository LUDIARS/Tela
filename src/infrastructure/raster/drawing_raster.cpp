// @spec Overlay drawing
#include <tela/drawing_raster.hpp>
#include "shape_distance.hpp"
#include "pixel_blend.hpp"
#include <algorithm>
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
Rect pixel_extent(const Shape& shape,Point origin,float scale,Rect clip) {
    auto bounds=raster::extent(shape);
    const float padding=shape.stroke.width/2+1/scale;
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
        if(shape.kind!=ShapeKind::polyline) color(pixel,shape.fill,.5f-distance*scale);
        if(shape.stroke.width>0) color(pixel,shape.stroke.color,
            .5f-(std::abs(distance)-shape.stroke.width/2)*scale);
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
