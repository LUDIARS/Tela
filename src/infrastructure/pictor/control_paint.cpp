// @spec SPEC-TL-CONTROLS
#include "control_paint.hpp"
#include "../raster/pixel_blend.hpp"
#include <tela/drawing_raster.hpp>
#include <algorithm>
#include <cmath>

namespace tela::pictor_adapter {
namespace {
Rect scaled(Rect r, float s) { return {r.x*s, r.y*s, r.width*s, r.height*s}; }
struct Painter {
    PixelSurface& out;
    PictorText& text;
    Rect clip;
    float scale;
    void operator()(const VisualRect& r) const {
        const auto px = scaled(r.rect, scale);
        if (r.border <= 0) { fill(out, px, clip, r.color); return; }
        const float w = std::max(1.f, std::round(r.border*scale));
        fill(out, {px.x, px.y, px.width, w}, clip, r.color);
        fill(out, {px.x, px.y+px.height-w, px.width, w}, clip, r.color);
        fill(out, {px.x, px.y, w, px.height}, clip, r.color);
        fill(out, {px.x+px.width-w, px.y, w, px.height}, clip, r.color);
    }
    void operator()(const VisualEllipse& e) const {
        Drawing shape;
        shape.ellipse({0, 0, e.rect.width, e.rect.height}, e.color);
        paint_drawing(out, shape, {e.rect.x, e.rect.y}, scale, clip);
    }
    void operator()(const VisualText& t) const {
        text.draw(out, t.text, t.origin.x*scale, t.origin.y*scale, t.font_size*scale, t.line_height*scale,
                  t.color, intersect(scaled(t.clip, scale), clip));
    }
};
}
void fill(PixelSurface& out, Rect rect, Rect clip, Color c) {
    const auto r = intersect(rect, clip);
    const int left = std::max(0, static_cast<int>(std::floor(r.x)));
    const int top = std::max(0, static_cast<int>(std::floor(r.y)));
    const int right = std::min(out.width, static_cast<int>(std::ceil(r.x+r.width)));
    const int bottom = std::min(out.height, static_cast<int>(std::ceil(r.y+r.height)));
    for (int y = top; y < bottom; ++y) for (int x = left; x < right; ++x)
        raster::blend(&out.pixels[(static_cast<std::size_t>(y)*out.width+x)*4],
                      c.b*c.a/255, c.g*c.a/255, c.r*c.a/255, c.a);
}
void paint_control(PixelSurface& out, PictorText& text, const Runtime& runtime, const PlacedElement& placed, Rect clip) {
    const float scale = runtime.viewport().dpi_scale;
    const auto& e = placed.element;
    const ControlLook look{placed, runtime.state(e.id), runtime.focused() == e.id, runtime.theme(), runtime.text_measure()};
    const Painter painter{out, text, clip, scale};
    for (const auto& item : control_visual(look)) std::visit(painter, item);
}
}
