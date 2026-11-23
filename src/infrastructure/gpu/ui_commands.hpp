#pragma once
// @implements SPEC-TL-GPU
// @spec GPU rendering
#include <tela/gpu_frame.hpp>
#include <pictor/ui/ui_renderer.h>
#include <cstdint>
#include <vector>

namespace tela::gpu {
struct LayerTextures {
    std::vector<std::uint32_t> ids; // UIRenderer texture id per layer index
    float width{}, height{};        // texture size; a layer covers it from the top-left
};
// The frame as Pictor UIRenderer commands: quads become Rect commands under PushClip scissors,
// layers become full-size Image commands with the scissor released. `linear_target` converts
// the sRGB theme colors for a *_SRGB attachment, which encodes on write.
std::vector<pictor::UIDrawCmd> ui_commands(const GpuFrame&, const LayerTextures&, bool linear_target);
}
