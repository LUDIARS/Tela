// @spec SPEC-TL-MACOS
#include "region_surface.hpp"
#include "bitmap_view.hpp"
#include <algorithm>
#include <cmath>

namespace tela::macos {
NSRect fragment_frame(Rect fragment, const WindowTarget& target) {
    const auto& v = target.viewport;
    const double scale = v.dpi_scale;
    const int x = std::clamp(static_cast<int>(std::ceil(fragment.x * scale)), 0, v.width);
    const int y = std::clamp(static_cast<int>(std::ceil(fragment.y * scale)), 0, v.height);
    const int right = std::clamp(static_cast<int>(std::ceil((fragment.x + fragment.width) * scale)), x, v.width);
    const int bottom = std::clamp(static_cast<int>(std::ceil((fragment.y + fragment.height) * scale)), y, v.height);
    return NSMakeRect(target.frame.origin.x + x / scale, NSMaxY(target.frame) - bottom / scale,
        (right - x) / scale, (bottom - y) / scale);
}
void present_regions(NSArray<NSPanel*>* panels, PixelSurface& visual, const WindowTarget& target) {
    const double scale = target.viewport.dpi_scale;
    for (NSPanel* panel in panels) {
        const NSRect frame = panel.frame;
        const int x = std::clamp(static_cast<int>(std::lround((frame.origin.x - target.frame.origin.x) * scale)), 0, visual.width);
        const int y = std::clamp(static_cast<int>(std::lround((NSMaxY(target.frame) - NSMaxY(frame)) * scale)), 0, visual.height);
        const int width = std::clamp(static_cast<int>(std::lround(frame.size.width * scale)), 0, visual.width - x);
        const int height = std::clamp(static_cast<int>(std::lround(frame.size.height * scale)), 0, visual.height - y);
        if (!width || !height) { [panel orderOut:nil]; continue; }
        PixelSurface cropped{width, height, std::vector<unsigned char>(static_cast<std::size_t>(width) * height * 4)};
        for (int row = 0; row < height; ++row) {
            for (int column = 0; column < width; ++column) {
                auto* source = &visual.pixels[(static_cast<std::size_t>(y + row) * visual.width + x + column) * 4];
                auto* dest = &cropped.pixels[(static_cast<std::size_t>(row) * width + column) * 4];
                std::copy_n(source, 4, dest);
                // AppKit ignores fully transparent pixels during mouse hit tests.
                // Match the Windows hit-surface contract: minimum 1/255 alpha
                // only in explicit exclusive fragments, never the pass-through area.
                dest[3] = std::max<unsigned char>(1, dest[3]);
                std::fill_n(source, 4, 0); // final pixel is composited exactly once
            }
        }
        present_bitmap(panel, cropped, frame, scale);
    }
}
}
