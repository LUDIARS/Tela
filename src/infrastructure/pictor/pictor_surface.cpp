// @spec SPEC-TL-OVERLAY
#include <tela/pictor_surface.hpp>
#include <pictor/text/text_image_renderer.h>
#include <cmath>
#include <stdexcept>

namespace tela {
struct PictorSurface::Impl {
    pictor::FontLoader fonts;
    pictor::TextImageRenderer text{fonts};
    pictor::FontHandle font;
    explicit Impl(const std::string& path) : font(fonts.load_from_file(path)) {
        if (font == pictor::INVALID_FONT) throw std::runtime_error("Cannot load Pictor TrueType font: " + path);
        const auto* entry = fonts.get_entry(font);
        if (!entry || entry->raw_data.size()<4 || entry->raw_data[0]!=0 || entry->raw_data[1]!=1)
            throw std::runtime_error("Tela requires TrueType outlines; CFF approximation is unsupported");
    }
};
namespace {
void blend(unsigned char* dst, unsigned char b, unsigned char g, unsigned char r, unsigned char a) {
    const unsigned inv = 255-a;
    dst[0] = static_cast<unsigned char>(std::min(255u,b+(dst[0]*inv+127)/255));
    dst[1] = static_cast<unsigned char>(std::min(255u,g+(dst[1]*inv+127)/255));
    dst[2] = static_cast<unsigned char>(std::min(255u,r+(dst[2]*inv+127)/255));
    dst[3] = static_cast<unsigned char>(std::min(255u,a+(dst[3]*inv+127)/255));
}
Rect pixels(Rect r, float scale) { return {r.x*scale,r.y*scale,r.width*scale,r.height*scale}; }
void rectangle(PixelSurface& out, Rect rect, Color c) {
    const int left = std::max(0,static_cast<int>(std::floor(rect.x)));
    const int top = std::max(0,static_cast<int>(std::floor(rect.y)));
    const int right = std::min(out.width,static_cast<int>(std::ceil(rect.x+rect.width)));
    const int bottom = std::min(out.height,static_cast<int>(std::ceil(rect.y+rect.height)));
    for (int y=top;y<bottom;++y) for (int x=left;x<right;++x)
        blend(&out.pixels[(static_cast<size_t>(y)*out.width+x)*4],
              c.b*c.a/255,c.g*c.a/255,c.r*c.a/255,c.a);
}
}
PictorSurface::PictorSurface(const std::string& font) : impl_(std::make_unique<Impl>(font)) {}
PictorSurface::~PictorSurface() = default;
PixelSurface PictorSurface::render(const Runtime& runtime) {
    const auto& view = runtime.viewport();
    PixelSurface out{view.width,view.height,std::vector<unsigned char>(static_cast<size_t>(view.width)*view.height*4)};
    const auto& theme = runtime.theme();
    for (const auto& placed : runtime.elements()) {
        const auto& e = placed.element;
        Rect clip = pixels(placed.clip,view.dpi_scale);
        if (clip.width <= 0 || clip.height <= 0) continue;
        if (e.kind == ElementKind::panel) rectangle(out,clip,theme.panel);
        if (e.kind == ElementKind::button) {
            auto state = runtime.state(e.id);
            rectangle(out,clip,state && state->pressed ? theme.pressed : theme.button);
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
        style.font_size = theme.font_size*view.dpi_scale;
        style.color = {theme.text.r/255.f,theme.text.g/255.f,theme.text.b/255.f,theme.text.a/255.f};
        style.align_v = pictor::TextAlignV::TOP;
        style.max_width = static_cast<float>(width);
        auto text = impl_->text.render_text_fixed(impl_->font,e.label,width,height,style);
        for (int y=0;y<height;++y) for (int x=0;x<width;++x) {
            const int dx=x0+x, dy=y0+y;
            if (dx<0 || dy<0 || dx>=out.width || dy>=out.height || !clip.contains(static_cast<float>(dx),static_cast<float>(dy))) continue;
            const auto* source = &text.pixels[(static_cast<size_t>(y)*width+x)*4];
            // Pictor composites premultiplied RGBA; swap R/B without multiplying twice.
            blend(&out.pixels[(static_cast<size_t>(dy)*out.width+dx)*4],source[2],source[1],source[0],source[3]);
        }
    }
    return out;
}
}
