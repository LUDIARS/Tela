// @spec SPEC-TL-CONTROLS
#include "pictor_text.hpp"
#include "../raster/pixel_blend.hpp"
#include <algorithm>
#include <cmath>
#include <string>

namespace tela::pictor_adapter {
namespace {
constexpr int line_width_limit = 16384;
pictor::TextStyle style(float font_px, Color color) {
    pictor::TextStyle s;
    s.font_size = font_px;
    s.color = {color.r/255.f, color.g/255.f, color.b/255.f, color.a/255.f};
    s.align_v = pictor::TextAlignV::TOP;
    s.max_width = 0;
    s.word_wrap = false;
    return s;
}
}
float PictorText::width(std::string_view text, float font_px) const {
    if (text.empty()) return 0;
    return renderer_.measure_text(font_, std::string(text), style(font_px, {})).width;
}
void PictorText::draw(PixelSurface& out, std::string_view text, float x, float y, float font_px, float line_px,
                      Color color, Rect clip) {
    if (text.empty() || clip.width <= 0 || clip.height <= 0) return;
    const int x0 = static_cast<int>(std::floor(x)), y0 = static_cast<int>(std::floor(y));
    // Rasterize only up to the clip's right edge; a long shifted line never allocates beyond it.
    const int right = static_cast<int>(std::ceil(clip.x + clip.width));
    const int width = std::min({static_cast<int>(std::ceil(this->width(text, font_px))) + 2, right - x0, line_width_limit});
    const int height = std::min(static_cast<int>(std::ceil(line_px)), out.height);
    if (width <= 0 || height <= 0) return;
    auto image = renderer_.render_text_fixed(font_, std::string(text), static_cast<std::uint32_t>(width),
                                             static_cast<std::uint32_t>(height), style(font_px, color));
    for (int row = 0; row < height; ++row) for (int column = 0; column < width; ++column) {
        const int dx = x0 + column, dy = y0 + row;
        if (dx < 0 || dy < 0 || dx >= out.width || dy >= out.height ||
            !clip.contains(static_cast<float>(dx), static_cast<float>(dy))) continue;
        const auto* source = &image.pixels[(static_cast<std::size_t>(row) * width + column) * 4];
        // Pictor composites premultiplied RGBA; swap R/B without multiplying twice.
        raster::blend(&out.pixels[(static_cast<std::size_t>(dy) * out.width + dx) * 4],
                      source[2], source[1], source[0], source[3]);
    }
}
}
