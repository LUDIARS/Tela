// @spec Overlay drawing
#include <tela/drawing.hpp>
#include <cmath>
#include <stdexcept>

namespace tela {
namespace {
void coordinate(float value) {
    if(!std::isfinite(value) || std::abs(value)>1000000)
        throw std::invalid_argument("Invalid drawing coordinate");
}
void validate(const Shape& shape) {
    for(float v:{shape.bounds.x,shape.bounds.y,shape.bounds.width,shape.bounds.height,
        shape.radius,shape.stroke.width,shape.stroke.dash,shape.stroke.gap}) coordinate(v);
    if(shape.bounds.width<0 || shape.bounds.height<0 || shape.radius<0 ||
        shape.stroke.width<0 || shape.stroke.width>256 || shape.stroke.dash<0 || shape.stroke.gap<0)
        throw std::invalid_argument("Invalid drawing dimensions");
    if((shape.stroke.dash==0)!=(shape.stroke.gap==0))
        throw std::invalid_argument("Dashed stroke requires both dash and gap");
    if(shape.kind!=ShapeKind::polyline && shape.stroke.dash!=0)
        throw std::invalid_argument("Dashed borders require an explicit polyline");
    for(auto point:shape.points) { coordinate(point.x); coordinate(point.y); }
}
}
void Drawing::append(Shape shape) {
    validate(shape);
    if(shapes_.size()>=256 || point_count_+shape.points.size()>4096)
        throw std::length_error("Drawing command budget exceeded");
    const auto count=shape.points.size();
    shapes_.push_back(std::move(shape)); point_count_+=count;
}
void Drawing::rectangle(Rect bounds,Color fill,Stroke stroke,float radius) {
    append({ShapeKind::rectangle,bounds,radius,fill,stroke,{}});
}
void Drawing::ellipse(Rect bounds,Color fill,Stroke stroke) {
    append({ShapeKind::ellipse,bounds,0,fill,stroke,{}});
}
void Drawing::line(Point from,Point to,Stroke stroke) { polyline({from,to},stroke); }
void Drawing::polyline(std::vector<Point> points,Stroke stroke) {
    if(points.size()<2) throw std::invalid_argument("Polyline requires two points");
    Shape shape; shape.kind=ShapeKind::polyline; shape.stroke=stroke;
    shape.points=std::move(points); append(std::move(shape));
}
}
