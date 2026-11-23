// @implements SPEC-TL-GPU
// @spec GPU rendering
#pragma once
#include <tela/drawing.hpp>
#include <tela/pixel_surface.hpp>
#include <tela/runtime.hpp>
#include <cstddef>
#include <string>
#include <variant>
#include <vector>

namespace tela {
// GPU-independent plan of one frame, in physical pixels and painting order. A GPU backend draws
// quads with its quad pipeline and composites layers it rasterized from the layer items; it
// decides nothing about the look (that is control_visual and the element kinds, as for the CPU
// raster and the web page).

// Solid rectangle, optionally rounded, limited to `clip` (an integer scissor).
struct GpuQuad {
    Rect rect;
    Color color;
    float radius{};
    Rect clip;
    bool operator==(const GpuQuad&) const = default;
};
// Composite layer `index` (its content is already clipped) at this point of the order.
struct GpuLayerDraw {
    std::size_t index{};
    bool operator==(const GpuLayerDraw&) const = default;
};
using GpuStep = std::variant<GpuQuad, GpuLayerDraw>;

// Text drawn into a layer. Unwrapped lines have their top-left at box.x/box.y and are one line
// (box.height) tall; wrapped labels fill the box as the CPU raster does.
struct LayerText {
    std::string text;
    Rect box;
    float font_px{}, line_px{};
    bool wrap{};
    Color color;
    Rect clip;
    bool operator==(const LayerText&) const = default;
};
// Shapes the quad pipeline cannot draw (ellipses, strokes, gradients, polylines, images) and
// quads that must stay above layer content once the layers run out. Drawn like the CPU raster:
// `origin` and the drawing are logical units multiplied by `scale`.
struct LayerShapes {
    Drawing drawing;
    Point origin;
    float scale{1};
    Rect clip;
    bool operator==(const LayerShapes&) const = default;
};
using LayerItem = std::variant<LayerText, LayerShapes>;
struct GpuLayer {
    std::vector<LayerItem> items;
    bool operator==(const GpuLayer&) const = default;
};
struct GpuFrame {
    int width{}, height{}; // physical viewport
    std::vector<GpuStep> steps;
    std::vector<GpuLayer> layers;
    bool operator==(const GpuFrame&) const = default;
};
struct GpuFrameOptions {
    // Layer textures the backend owns. A quad that would cover content of the last layer is
    // drawn into that layer instead, so the order stays exact with any count of at least one.
    std::size_t max_layers{3};
};
// A local reveal over the new content. The host owns the frame clock and hit testing stays live.
class GpuReveal {
public:
    void begin(bool reduced_motion) { remaining_ = reduced_motion ? 0.f : duration; }
    void advance(float seconds) { remaining_ = std::max(0.f, remaining_ - std::max(0.f, seconds)); }
    bool active() const { return remaining_ > 0.f; }
    unsigned char alpha() const {
        const float t = remaining_ / duration;
        return static_cast<unsigned char>(96.f * t * t);
    }
private:
    static constexpr float duration = 0.12f;
    float remaining_{};
};
// Physical scissor of a logical clip: the pixels the CPU raster's clip test admits.
Rect gpu_clip(Rect logical, float dpi_scale);
GpuFrame plan_gpu_frame(const Runtime&, const GpuFrameOptions& = {});
// Texels of a rasterized layer for a straight-alpha RGBA texture whose rows are `row_pixels`
// wide (at least the surface width): the surface's premultiplied BGRA, un-premultiplied.
void layer_texels(const PixelSurface&, unsigned char* rgba, int row_pixels);
}
