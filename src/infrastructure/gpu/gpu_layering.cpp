// @spec SPEC-TL-GPU
#include "gpu_layering.hpp"
#include <algorithm>

namespace tela::gpu {
namespace {
bool empty(Rect r) { return r.width <= 0 || r.height <= 0; }
}
Layering::Layering(GpuFrame& frame, std::size_t max_layers) : frame_(frame), max_layers_(std::max<std::size_t>(1, max_layers)) {}

bool Layering::covers(Rect area) const {
    return std::any_of(areas_.begin(), areas_.end(), [&](Rect shown) { return !empty(intersect(shown, area)); });
}
void Layering::close() {
    frame_.steps.push_back(GpuLayerDraw{frame_.layers.size() - 1});
    areas_.clear();
    open_ = false;
}
void Layering::quad(Rect rect, Color color, float radius, Rect clip) {
    const auto area = intersect(rect, clip);
    if (empty(area) || color.a == 0) return;
    if (open_ && covers(area)) {
        if (frame_.layers.size() < max_layers_) {
            close();
        } else {
            // No layer left to put above this quad: the last layer draws it, so it still covers
            // what came before and stays under what comes after.
            Drawing fill;
            fill.rectangle(rect, color, {{0, 0, 0, 0}, 0}, radius);
            add(LayerShapes{std::move(fill), {}, 1, clip}, area);
            return;
        }
    }
    frame_.steps.push_back(GpuQuad{rect, color, radius, clip});
}
void Layering::text(LayerText text) {
    if (text.text.empty()) return;
    // An unwrapped line runs to its natural width, which only the font knows; its clip bounds it.
    const Rect reach = text.wrap ? text.box
                                 : Rect{text.box.x, text.box.y, text.clip.x + text.clip.width - text.box.x, text.box.height};
    const auto area = intersect(reach, text.clip);
    if (empty(area)) return;
    add(std::move(text), area);
}
void Layering::shapes(LayerShapes shapes, Rect area) {
    area = intersect(area, shapes.clip);
    if (empty(area) || shapes.drawing.shapes().empty()) return;
    add(std::move(shapes), area);
}
void Layering::add(LayerItem item, Rect area) {
    if (!open_) {
        frame_.layers.emplace_back();
        open_ = true;
    }
    frame_.layers.back().items.push_back(std::move(item));
    areas_.push_back(area);
}
void Layering::finish() {
    if (open_) close();
}
}
