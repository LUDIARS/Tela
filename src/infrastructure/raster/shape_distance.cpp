// @spec Overlay drawing
#include "shape_distance.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

namespace tela::raster {
namespace {
float rounded_rectangle(const Shape& shape,Point p) {
    const auto b=shape.bounds;
    const float radius=std::min({shape.radius,b.width/2,b.height/2});
    const float x=std::abs(p.x-b.x-b.width/2)-b.width/2+radius;
    const float y=std::abs(p.y-b.y-b.height/2)-b.height/2+radius;
    return std::hypot(std::max(x,0.f),std::max(y,0.f))+std::min(std::max(x,y),0.f)-radius;
}
float ellipse(const Shape& shape,Point p) {
    const auto b=shape.bounds;
    const double rx=static_cast<double>(b.width)/2,ry=static_cast<double>(b.height)/2;
    if(rx==0||ry==0) return std::numeric_limits<float>::infinity();
    const double x=p.x-b.x-rx,y=p.y-b.y-ry;
    const double k0=std::hypot(x/rx,y/ry),k1=std::hypot(x/(rx*rx),y/(ry*ry));
    return static_cast<float>(k1==0?-std::min(rx,ry):k0*(k0-1)/k1);
}
float segment(Point p,Point a,Point b,float prefix,const Stroke& stroke) {
    const float dx=b.x-a.x,dy=b.y-a.y,length=std::hypot(dx,dy);
    const float t=length==0?0:std::clamp(((p.x-a.x)*dx+(p.y-a.y)*dy)/(length*length),0.f,1.f);
    if(stroke.dash>0&&std::fmod(prefix+t*length,stroke.dash+stroke.gap)>=stroke.dash)
        return std::numeric_limits<float>::infinity();
    return std::hypot(p.x-a.x-t*dx,p.y-a.y-t*dy);
}
float polyline(const Shape& shape,Point p) {
    float result=std::numeric_limits<float>::infinity(),prefix=0;
    for(std::size_t i=1;i<shape.points.size();++i) {
        const auto a=shape.points[i-1],b=shape.points[i];
        result=std::min(result,segment(p,a,b,prefix,shape.stroke));
        prefix+=std::hypot(b.x-a.x,b.y-a.y);
    }
    return result;
}
}
float distance(const Shape& shape,Point p) {
    switch(shape.kind) {
    case ShapeKind::rectangle: return rounded_rectangle(shape,p);
    case ShapeKind::ellipse: return ellipse(shape,p);
    case ShapeKind::polyline: return polyline(shape,p);
    }
    return std::numeric_limits<float>::infinity();
}
Rect extent(const Shape& shape) {
    if(shape.kind!=ShapeKind::polyline) return shape.bounds;
    if(shape.points.empty()) return {};
    float left=shape.points.front().x,right=left,top=shape.points.front().y,bottom=top;
    for(auto p:shape.points) { left=std::min(left,p.x);right=std::max(right,p.x);top=std::min(top,p.y);bottom=std::max(bottom,p.y); }
    return {left,top,right-left,bottom-top};
}
}
