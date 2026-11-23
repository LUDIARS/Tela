// @spec SPEC-TL-GPU
#include <tela/gpu_frame.hpp>
#include <cstddef>

namespace tela {
void layer_texels(const PixelSurface& surface, unsigned char* rgba, int row_pixels) {
    for (int y = 0; y < surface.height; ++y) {
        const auto* in = &surface.pixels[static_cast<std::size_t>(y) * surface.width * 4];
        auto* out = rgba + static_cast<std::size_t>(y) * row_pixels * 4;
        for (int x = 0; x < surface.width; ++x, in += 4, out += 4) {
            const unsigned a = in[3];
            // The quad pipeline blends straight alpha (source alpha, one minus source alpha).
            const auto straight = [a](unsigned c) {
                if (!a) return static_cast<unsigned char>(0);
                const unsigned value = (c * 255 + a / 2) / a;
                return static_cast<unsigned char>(value > 255 ? 255 : value);
            };
            out[0] = straight(in[2]);
            out[1] = straight(in[1]);
            out[2] = straight(in[0]);
            out[3] = static_cast<unsigned char>(a);
        }
    }
}
}
