// @spec SPEC-TL-GPU
#include "layer_raster.hpp"
#include <tela/drawing_raster.hpp>
#include <algorithm>

namespace tela::gpu {
void raster_layer(PixelSurface& out, const GpuLayer& layer, pictor_adapter::PictorText& text) {
    std::fill(out.pixels.begin(), out.pixels.end(), static_cast<unsigned char>(0));
    for (const auto& item : layer.items) {
        if (const auto* shapes = std::get_if<LayerShapes>(&item)) {
            paint_drawing(out, shapes->drawing, shapes->origin, shapes->scale, shapes->clip);
            continue;
        }
        const auto& t = std::get<LayerText>(item);
        if (t.wrap) text.draw_box(out, t.text, t.box, t.font_px, t.color, t.clip);
        else text.draw(out, t.text, t.box.x, t.box.y, t.font_px, t.line_px, t.color, t.clip);
    }
}
}
