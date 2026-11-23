// @spec SPEC-TL-GPU
#include <tela/gpu_frame.hpp>
#include <tela/control_visual.hpp>
#include "gpu_layering.hpp"
#include <algorithm>
#include <cmath>

namespace tela {
namespace {
Rect scaled(Rect r, float s) { return {r.x*s, r.y*s, r.width*s, r.height*s}; }
// Whole pixels a CPU fill touches: the rectangle grown outward to pixel edges.
Rect covered(Rect r) {
    const float left = std::floor(r.x), top = std::floor(r.y);
    return {left, top, std::ceil(r.x+r.width)-left, std::ceil(r.y+r.height)-top};
}
// Pixels whose top-left corner the CPU clip test admits.
Rect admitted(Rect r) {
    const float left = std::ceil(r.x), top = std::ceil(r.y);
    return {left, top, std::max(0.f, std::ceil(r.x+r.width)-left), std::max(0.f, std::ceil(r.y+r.height)-top)};
}
bool plain_fill(const Shape& s) {
    const bool stroked = s.stroke.width > 0 && s.stroke.color.a > 0;
    const bool glowing = s.stroke.glow.radius > 0 && s.stroke.glow.color.a > 0;
    return s.kind == ShapeKind::rectangle && !s.gradient && !stroked && !glowing && s.opacity == 1;
}
// Re-declares one shape into a drawing; Drawing only grows through its declaration calls.
void redeclare(Drawing& out, const Shape& s) {
    switch (s.kind) {
    case ShapeKind::rectangle:
        if (s.gradient) out.gradient_rectangle(s.bounds, *s.gradient, s.stroke, s.radius);
        else out.rectangle(s.bounds, s.fill, s.stroke, s.radius);
        break;
    case ShapeKind::ellipse:
        if (s.gradient) out.gradient_ellipse(s.bounds, *s.gradient, s.stroke);
        else out.ellipse(s.bounds, s.fill, s.stroke);
        break;
    case ShapeKind::polyline: out.polyline(s.points, s.stroke); break;
    case ShapeKind::image: out.image(s.bounds, s.image, s.opacity); break;
    }
}

class Planner {
public:
    Planner(const Runtime& runtime, GpuFrame& frame, std::size_t layers)
        : runtime_(runtime), layering_(frame, layers), scale_(runtime.viewport().dpi_scale) {}
    void element(const PlacedElement& placed) {
        const Rect clip = gpu_clip(placed.clip, scale_);
        if (clip.width <= 0 || clip.height <= 0) return;
        const auto& e = placed.element;
        if (is_control(e.kind)) { control(placed, clip); return; }
        if (e.kind == ElementKind::canvas) canvas(placed, clip);
        const auto& theme = runtime_.theme();
        if (e.kind == ElementKind::panel) layering_.quad(clip, theme.panel, 0, clip);
        if (e.kind == ElementKind::button) {
            const auto* state = runtime_.state(e.id);
            layering_.quad(clip, state && state->pressed ? theme.pressed : theme.button, 0, clip);
        }
        if (!e.label.empty()) label(placed, clip);
    }
    void finish() { layering_.finish(); }
private:
    const Runtime& runtime_;
    gpu::Layering layering_;
    float scale_;

    void fill(Rect logical, Color color, Rect clip) { layering_.quad(covered(scaled(logical, scale_)), color, 0, clip); }
    void control(const PlacedElement& placed, Rect clip) {
        const auto& e = placed.element;
        const ControlLook look{placed, runtime_.state(e.id), runtime_.focused() == e.id, runtime_.theme(), runtime_.text_measure()};
        for (const auto& item : control_visual(look)) {
            if (const auto* r = std::get_if<VisualRect>(&item)) rect(*r, clip);
            else if (const auto* o = std::get_if<VisualEllipse>(&item)) ellipse(*o, clip);
            else text(std::get<VisualText>(item), clip);
        }
    }
    void rect(const VisualRect& r, Rect clip) {
        if (r.border <= 0) { fill(r.rect, r.color, clip); return; }
        const auto px = scaled(r.rect, scale_);
        const float w = std::max(1.f, std::round(r.border*scale_));
        for (Rect side : {Rect{px.x, px.y, px.width, w}, Rect{px.x, px.y+px.height-w, px.width, w},
                          Rect{px.x, px.y, w, px.height}, Rect{px.x+px.width-w, px.y, w, px.height}})
            layering_.quad(covered(side), r.color, 0, clip);
    }
    void ellipse(const VisualEllipse& o, Rect clip) {
        if (o.rect.width == o.rect.height) { // a circle is a fully rounded quad
            const auto px = scaled(o.rect, scale_);
            layering_.quad(px, o.color, px.width/2, clip);
            return;
        }
        Drawing shape;
        shape.ellipse({0, 0, o.rect.width, o.rect.height}, o.color);
        layering_.shapes({std::move(shape), {o.rect.x, o.rect.y}, scale_, clip}, scaled(o.rect, scale_));
    }
    void text(const VisualText& t, Rect clip) {
        const Rect box{t.origin.x*scale_, t.origin.y*scale_, 0, t.line_height*scale_};
        layering_.text({t.text, box, t.font_size*scale_, t.line_height*scale_, false, t.color,
                        intersect(gpu_clip(t.clip, scale_), clip)});
    }
    void canvas(const PlacedElement& placed, Rect clip) {
        const Point origin{placed.bounds.x, placed.bounds.y};
        Drawing pending;
        const auto flush = [&] {
            if (pending.shapes().empty()) return;
            layering_.shapes({std::move(pending), origin, scale_, clip}, clip);
            pending = Drawing{};
        };
        for (const auto& s : placed.element.drawing.shapes()) {
            if (!plain_fill(s)) { redeclare(pending, s); continue; }
            flush();
            const Rect local{origin.x+s.bounds.x, origin.y+s.bounds.y, s.bounds.width, s.bounds.height};
            layering_.quad(scaled(local, scale_), s.fill, s.radius*scale_, clip);
        }
        flush();
    }
    // Wrapped label inside the padding, placed exactly as the CPU raster places it.
    void label(const PlacedElement& placed, Rect clip) {
        const auto& e = placed.element;
        const auto& view = runtime_.viewport();
        const float padding = e.layout.padding*scale_;
        const auto bounds = scaled(placed.bounds, scale_);
        const float width = static_cast<float>(std::min(view.width, std::max(0, static_cast<int>(bounds.width-2*padding))));
        const float height = static_cast<float>(std::min(view.height, std::max(0, static_cast<int>(bounds.height-2*padding))));
        if (width <= 0 || height <= 0) return;
        const auto& theme = runtime_.theme();
        const Rect box{std::floor(bounds.x+padding), std::floor(bounds.y+padding), width, height};
        layering_.text({e.label, box, theme.font_size*e.layout.text_scale*scale_, theme.line_height*e.layout.text_scale*scale_,
                        true, theme.text, clip});
    }
};
}
Rect gpu_clip(Rect logical, float dpi_scale) { return admitted(scaled(logical, dpi_scale)); }
GpuFrame plan_gpu_frame(const Runtime& runtime, const GpuFrameOptions& options) {
    GpuFrame frame;
    frame.width = runtime.viewport().width;
    frame.height = runtime.viewport().height;
    Planner planner(runtime, frame, options.max_layers);
    for (const auto& placed : runtime.elements()) planner.element(placed);
    planner.finish();
    return frame;
}
}
