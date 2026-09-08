#pragma once
#include <tela/geometry.hpp>
#include <vector>

namespace tela {
struct Point { float x{}, y{}; bool operator==(const Point&) const = default; };
struct Stroke {
    Color color{255,255,255,255};
    float width{1}, dash{}, gap{}; // logical pixels; dash/gap both zero means solid
    bool operator==(const Stroke&) const = default;
};
enum class ShapeKind { rectangle, ellipse, polyline };
struct Shape {
    ShapeKind kind{};
    Rect bounds;
    float radius{};
    Color fill{0,0,0,0};
    Stroke stroke;
    std::vector<Point> points;
    bool operator==(const Shape&) const = default;
};
// @spec Overlay drawing
// Value declaration in local logical coordinates. Backends own rasterization.
class Drawing {
public:
    void rectangle(Rect,Color fill,Stroke stroke={{0,0,0,0},0},float radius=0);
    void ellipse(Rect,Color fill,Stroke stroke={{0,0,0,0},0});
    void line(Point from,Point to,Stroke);
    void polyline(std::vector<Point>,Stroke);
    const std::vector<Shape>& shapes() const noexcept { return shapes_; }
    bool operator==(const Drawing&) const = default;
private:
    void append(Shape);
    std::vector<Shape> shapes_;
    std::size_t point_count_{};
};
}
