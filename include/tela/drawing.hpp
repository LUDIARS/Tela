#pragma once
#include <tela/geometry.hpp>
#include <memory>
#include <optional>
#include <vector>

namespace tela {
struct Point { float x{}, y{}; bool operator==(const Point&) const = default; };
// Soft light outside a stroke edge; zero radius or transparent color means none.
struct Glow {
    Color color{0,0,0,0};
    float radius{}; // logical pixels beyond the stroke edge
    bool operator==(const Glow&) const = default;
};
struct Stroke {
    Color color{255,255,255,255};
    float width{1}, dash{}, gap{}; // logical pixels; dash/gap both zero means solid
    Glow glow{};
    bool operator==(const Stroke&) const = default;
};
// Linear fill in the shape's local logical coordinates; colors clamp beyond start/end.
struct LinearGradient {
    Point start, end;
    Color from, to;
    bool operator==(const LinearGradient&) const = default;
};
// Top-down premultiplied BGRA8, width*4 stride. Shared so redeclaring a frame never copies pixels;
// equality is identity, so a new image object is what invalidates a canvas.
struct Image {
    int width{}, height{};
    std::vector<unsigned char> pixels;
};
enum class ShapeKind { rectangle, ellipse, polyline, image };
struct Shape {
    ShapeKind kind{};
    Rect bounds;
    float radius{};
    Color fill{0,0,0,0};
    Stroke stroke;
    std::vector<Point> points;
    std::optional<LinearGradient> gradient;
    std::shared_ptr<const Image> image;
    float opacity{1};
    bool operator==(const Shape&) const = default;
};
// @spec Overlay drawing
// Value declaration in local logical coordinates. Backends own rasterization.
class Drawing {
public:
    void rectangle(Rect,Color fill,Stroke stroke={{0,0,0,0},0},float radius=0);
    void gradient_rectangle(Rect,LinearGradient fill,Stroke stroke={{0,0,0,0},0},float radius=0);
    void ellipse(Rect,Color fill,Stroke stroke={{0,0,0,0},0});
    void gradient_ellipse(Rect,LinearGradient fill,Stroke stroke={{0,0,0,0},0});
    void line(Point from,Point to,Stroke);
    void polyline(std::vector<Point>,Stroke);
    void image(Rect,std::shared_ptr<const Image>,float opacity=1);
    const std::vector<Shape>& shapes() const noexcept { return shapes_; }
    bool operator==(const Drawing&) const = default;
private:
    void append(Shape);
    std::vector<Shape> shapes_;
    std::size_t point_count_{};
};
// Leading part of a polyline by arc length, `progress` in [0,1]; used for path reveal animation.
// Returns fewer than two points (nothing to draw) at zero progress.
std::vector<Point> trim_polyline(const std::vector<Point>& points,float progress);
}
