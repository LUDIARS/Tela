// @spec SPEC-TL-OVERLAY
#include <tela/pictor_surface.hpp>
#include <tela/drawing_raster.hpp>
#include "control_paint.hpp"
#include "../raster/pixel_blend.hpp"
#include <pictor/text/text_image_renderer.h>
#include <cmath>
#include <stdexcept>

namespace tela {
struct PictorSurface::Impl {
    pictor::FontLoader fonts;
    pictor::TextImageRenderer text{fonts};
    pictor::FontHandle font;
    pictor_adapter::PictorText line{text, font};
    explicit Impl(const std::string& path) : font(fonts.load_from_file(path)), line(text, font) {
        if (font == pictor::INVALID_FONT) throw std::runtime_error("Cannot load Pictor TrueType font: " + path);
        const auto* entry = fonts.get_entry(font);
        if (!entry || entry->raw_data.size()<4 || entry->raw_data[0]!=0 || entry->raw_data[1]!=1)
            throw std::runtime_error("Tela requires TrueType outlines; CFF approximation is unsupported");
    }
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
        pictor::TextStyle style;
        style.font_size = theme.font_size*e.layout.text_scale*view.dpi_scale;
        style.color = {theme.text.r/255.f,theme.text.g/255.f,theme.text.b/255.f,theme.text.a/255.f};
        style.align_v = pictor::TextAlignV::TOP;
        style.max_width = static_cast<float>(width);
        auto text = impl_->text.render_text_fixed(impl_->font,e.label,width,height,style);
        for (int y=0;y<height;++y) for (int x=0;x<width;++x) {
            const int dx=x0+x, dy=y0+y;
            if (dx<0 || dy<0 || dx>=out.width || dy>=out.height || !clip.contains(static_cast<float>(dx),static_cast<float>(dy))) continue;
            const auto* source = &text.pixels[(static_cast<size_t>(y)*width+x)*4];
            // Pictor composites premultiplied RGBA; swap R/B without multiplying twice.
            raster::blend(&out.pixels[(static_cast<size_t>(dy)*out.width+dx)*4],source[2],source[1],source[0],source[3]);
        }
    }
    return out;
}
}
