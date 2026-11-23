#pragma once
// @implements SPEC-TL-GPU
// @spec GPU rendering
#include <tela/gpu_frame.hpp>

namespace tela::gpu {
// Keeps a frame's painting order exact while splitting it between GPU quads and CPU layers.
// Items join the open layer; a quad that would cover any of them closes the layer (its draw
// step comes first) or, when no layer is left, is drawn into the last layer itself.
class Layering {
public:
    Layering(GpuFrame& frame, std::size_t max_layers);
    // `rect` and `clip` are physical; nothing is added when they do not meet.
    void quad(Rect rect, Color color, float radius, Rect clip);
    void text(LayerText);
    void shapes(LayerShapes, Rect area);
    // Draws the open layer; call once after the last item.
    void finish();
private:
    void add(LayerItem, Rect area);
    bool covers(Rect area) const;
    void close();
    GpuFrame& frame_;
    std::size_t max_layers_;
    std::vector<Rect> areas_; // what the open layer already shows
    bool open_{};
};
}
