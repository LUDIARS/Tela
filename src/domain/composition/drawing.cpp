// @spec Overlay drawing
#include <tela/drawing.hpp>
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace tela {
namespace {
constexpr Color canvas_base{26, 31, 42, 235};
constexpr float accent_mix = 0.19f;
constexpr unsigned char mix(unsigned char base, unsigned char accent) {
    return static_cast<unsigned char>(base + (accent - base) * accent_mix + 0.5f);
}
}
Color tint(Color accent) {
    return {mix(canvas_base.r, accent.r), mix(canvas_base.g, accent.g),
            mix(canvas_base.b, accent.b), canvas_base.a};
}
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
    coordinate(shape.stroke.glow.radius);
    if(shape.stroke.glow.radius<0 || shape.stroke.glow.radius>128)
        throw std::invalid_argument("Invalid glow radius");
    coordinate(shape.opacity);
    if(shape.opacity<0 || shape.opacity>1) throw std::invalid_argument("Invalid opacity");
    if(shape.gradient) for(auto point:{shape.gradient->start,shape.gradient->end}) { coordinate(point.x); coordinate(point.y); }
    for(auto point:shape.points) { coordinate(point.x); coordinate(point.y); }
}
void validate(const Image& image) {
    if(image.width<=0 || image.height<=0 || image.width>4096 || image.height>4096 ||
        image.pixels.size()!=static_cast<std::size_t>(image.width)*image.height*4)
        throw std::invalid_argument("Invalid image");
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
void Drawing::gradient_rectangle(Rect bounds,LinearGradient fill,Stroke stroke,float radius) {
    Shape shape{ShapeKind::rectangle,bounds,radius,{0,0,0,0},stroke,{}};
    shape.gradient=fill; append(std::move(shape));
}
void Drawing::ellipse(Rect bounds,Color fill,Stroke stroke) {
    append({ShapeKind::ellipse,bounds,0,fill,stroke,{}});
}
void Drawing::gradient_ellipse(Rect bounds,LinearGradient fill,Stroke stroke) {
    Shape shape{ShapeKind::ellipse,bounds,0,{0,0,0,0},stroke,{}};
    shape.gradient=fill; append(std::move(shape));
}
void Drawing::image(Rect bounds,std::shared_ptr<const Image> pixels,float opacity) {
    if(!pixels) throw std::invalid_argument("Image is required");
    validate(*pixels);
    Shape shape; shape.kind=ShapeKind::image; shape.bounds=bounds; shape.stroke={{0,0,0,0},0};
    shape.image=std::move(pixels); shape.opacity=opacity; append(std::move(shape));
}
std::vector<Point> trim_polyline(const std::vector<Point>& points,float progress) {
    if(!std::isfinite(progress)) throw std::invalid_argument("Invalid trim progress");
    progress=std::clamp(progress,0.f,1.f);
    float total=0;
    for(std::size_t i=1;i<points.size();++i) total+=std::hypot(points[i].x-points[i-1].x,points[i].y-points[i-1].y);
    if(points.size()<2 || progress<=0 || total<=0) return {};
    if(progress>=1) return points;
    float remaining=total*progress;
    std::vector<Point> result{points.front()};
    for(std::size_t i=1;i<points.size();++i) {
        const auto a=points[i-1],b=points[i];
        const float length=std::hypot(b.x-a.x,b.y-a.y);
        if(length>=remaining) {
            const float t=length==0?0:remaining/length;
            result.push_back({a.x+(b.x-a.x)*t,a.y+(b.y-a.y)*t});
            return result;
        }
        result.push_back(b); remaining-=length;
    }
    return result;
}
void Drawing::line(Point from,Point to,Stroke stroke) { polyline({from,to},stroke); }
void Drawing::polyline(std::vector<Point> points,Stroke stroke) {
    if(points.size()<2) throw std::invalid_argument("Polyline requires two points");
    Shape shape; shape.kind=ShapeKind::polyline; shape.stroke=stroke;
    shape.points=std::move(points); append(std::move(shape));
}
}
