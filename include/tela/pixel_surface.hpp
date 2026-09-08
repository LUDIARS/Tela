#pragma once
#include <vector>
namespace tela {
// @spec Overlay drawing
// Top-down premultiplied BGRA8, width*4 stride. No GPU handles cross this API.
struct PixelSurface {
    int width{},height{};
    std::vector<unsigned char> pixels;
};
}
