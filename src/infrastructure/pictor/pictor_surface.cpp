// @spec SPEC-TL-OVERLAY
#include <tela/pictor_surface.hpp>
#include <tela/drawing_raster.hpp>
#include "control_paint.hpp"
#include <pictor/text/text_image_renderer.h>
#include <cmath>
#include <stdexcept>

namespace tela {
struct PictorSurface::Impl {
    pictor::FontLoader fonts;
    pictor::TextImageRenderer text{fonts};
    pictor::FontHandle font;
    pictor_adapter::PictorText line{text, font};
    explicit Impl(const std::string& path) : font(pictor_adapter::load_true_type(fonts, path)), line(text, font) {}
};
namespace {
Rect pixels(Rect r, float scale) { return {r.x*scale,r.y*scale,r.width*scale,r.height*scale}; }
}
PictorSurface::PictorSurface(const std::string& font) : impl_(std::make_unique<Impl>(font)) {}
PictorSurface::~PictorSurface() = default;
TextMeasure PictorSurface::measure() const {
    const auto* line = &impl_->line;
    return [line](std::string_view text, float font_size) { return line->width(text, font_size); };
}
PixelSurface PictorSurface::render(const Runtime& runtime) {
    const auto& view = runtime.viewport();
    PixelSurface out{view.width,view.height,std::vector<unsigned char>(static_cast<size_t>(view.width)*view.height*4)};
    const auto& theme = runtime.theme();
    for (const auto& placed : runtime.elements()) {
        const auto& e = placed.element;
        Rect clip = pixels(placed.clip,view.dpi_scale);
        if (clip.width <= 0 || clip.height <= 0) continue;
        if (is_control(e.kind)) {
            pictor_adapter::paint_control(out, impl_->line, runtime, placed, clip);
            continue;
        }
        if(e.kind==ElementKind::canvas)
            paint_drawing(out,e.drawing,{placed.bounds.x,placed.bounds.y},view.dpi_scale,clip);
        if (e.kind == ElementKind::panel) pictor_adapter::fill(out,clip,clip,theme.panel);
        if (e.kind == ElementKind::button) {
            auto state = runtime.state(e.id);
            pictor_adapter::fill(out,clip,clip,state && state->pressed ? theme.pressed : theme.button);
        }
        if (e.label.empty()) continue;
        const float padding = e.layout.padding*view.dpi_scale;
        const auto bounds = pixels(placed.bounds,view.dpi_scale);
        const int x0 = static_cast<int>(std::floor(bounds.x+padding));
        const int y0 = static_cast<int>(std::floor(bounds.y+padding));
        const int width = std::min(view.width,std::max(0,static_cast<int>(bounds.width-2*padding)));
        const int height = std::min(view.height,std::max(0,static_cast<int>(bounds.height-2*padding)));
        if (!width || !height) continue;
        impl_->line.draw_box(out, e.label,
                             {static_cast<float>(x0), static_cast<float>(y0), static_cast<float>(width), static_cast<float>(height)},
                             theme.font_size*e.layout.text_scale*view.dpi_scale, theme.text, clip);
    }
    return out;
}
}
