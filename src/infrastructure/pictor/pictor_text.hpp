#pragma once
// @implements SPEC-TL-CONTROLS
// @spec Controls
#include <tela/geometry.hpp>
#include <tela/pixel_surface.hpp>
#include <pictor/text/text_image_renderer.h>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace tela::pictor_adapter {
// Loads a TrueType-outline font; throws std::runtime_error for a missing file or CFF outlines.
pictor::FontHandle load_true_type(pictor::FontLoader&, const std::string& path);
// One unwrapped line of text through Pictor's TrueType rasterizer, in physical pixels.
class PictorText {
public:
    PictorText(pictor::TextImageRenderer& renderer, pictor::FontHandle font) : renderer_(renderer), font_(font) {}
    float width(std::string_view text, float font_px) const;
    // Draws with the line's top-left at (x, y), clipped to `clip`.
    void draw(PixelSurface&, std::string_view text, float x, float y, float font_px, float line_px,
              Color, Rect clip);
    // Wraps inside the whole-pixel `box` (a label inside its padding), clipped to `clip`.
    void draw_box(PixelSurface&, std::string_view text, Rect box, float font_px, Color, Rect clip);
private:
    static void blit(PixelSurface&, const std::vector<std::uint8_t>& pixels, int x, int y, int width, int height,
                     Rect clip);
    pictor::TextImageRenderer& renderer_;
    pictor::FontHandle font_;
};
}
