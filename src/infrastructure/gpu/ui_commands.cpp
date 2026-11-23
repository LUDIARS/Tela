// @spec SPEC-TL-GPU
#include "ui_commands.hpp"
#include <cmath>
#include <optional>

namespace tela::gpu {
namespace {
float channel(unsigned char c, bool linear) {
    const float v = c / 255.f;
    if (!linear) return v;
    return v <= 0.04045f ? v / 12.92f : std::pow((v + 0.055f) / 1.055f, 2.4f);
}
pictor::UIDrawCmd command(pictor::UIDrawKind kind, Rect r) {
    pictor::UIDrawCmd c;
    c.kind = kind;
    c.x = r.x;
    c.y = r.y;
    c.w = r.width;
    c.h = r.height;
    return c;
}
}
std::vector<pictor::UIDrawCmd> ui_commands(const GpuFrame& frame, const LayerTextures& layers, bool linear_target) {
    std::vector<pictor::UIDrawCmd> out;
    out.reserve(frame.steps.size() * 2);
    std::optional<Rect> scissor;
    for (const auto& step : frame.steps) {
        if (const auto* quad = std::get_if<GpuQuad>(&step)) {
            if (!scissor || !(*scissor == quad->clip)) {
                out.push_back(command(pictor::UIDrawKind::PushClip, quad->clip));
                scissor = quad->clip;
            }
            auto c = command(pictor::UIDrawKind::Rect, quad->rect);
            c.r = channel(quad->color.r, linear_target);
            c.g = channel(quad->color.g, linear_target);
            c.b = channel(quad->color.b, linear_target);
            c.a = quad->color.a / 255.f;
            c.corner_radius = quad->radius;
            out.push_back(c);
            continue;
        }
        const auto index = std::get<GpuLayerDraw>(step).index;
        if (index >= layers.ids.size()) continue;
        if (scissor) {
            out.push_back(command(pictor::UIDrawKind::PopClip, {}));
            scissor.reset();
        }
        auto c = command(pictor::UIDrawKind::Image, {0, 0, layers.width, layers.height});
        c.texture_id = layers.ids[index];
        out.push_back(c);
    }
    return out;
}
}
