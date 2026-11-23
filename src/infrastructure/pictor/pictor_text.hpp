#pragma once
// @implements SPEC-TL-CONTROLS
// @spec Controls
#include <tela/geometry.hpp>
#include <tela/pixel_surface.hpp>
#include <pictor/text/text_image_renderer.h>
#include <string_view>

namespace tela::pictor_adapter {
// One unwrapped line of text through Pictor's TrueType rasterizer, in physical pixels.
class PictorText {
public:
    PictorText(pictor::TextImageRenderer& renderer, pictor::FontHandle font) : renderer_(renderer), font_(font) {}
    float width(std::string_view text, float font_px) const;
    // Draws with the line's top-left at (x, y), clipped to `clip`.
    void draw(PixelSurface&, std::string_view text, float x, float y, float font_px, float line_px,
              Color, Rect clip);
private:
    pictor::TextImageRenderer& renderer_;
    pictor::FontHandle font_;
};
}
