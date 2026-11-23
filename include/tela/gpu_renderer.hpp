// @implements SPEC-TL-GPU
// @spec GPU rendering
#pragma once
#include <tela/gpu_frame.hpp>
#include <tela/runtime.hpp>
#include <cstdint>
#include <memory>
#include <string>
#include <vulkan/vulkan.h>

namespace pictor { class VulkanContext; }

namespace tela {
// Where the host draws: its Pictor Vulkan context, the render pass it records Tela into and the
// format of that pass's color attachment. The host keeps the window, the swapchain and the frame
// loop, so any window system that owns a Pictor VulkanContext can draw Tela.
struct GpuTarget {
    pictor::VulkanContext& vulkan;
    VkRenderPass render_pass{VK_NULL_HANDLE};
    VkFormat format{VK_FORMAT_B8G8R8A8_SRGB};
    std::string shader_dir; // Pictor's compiled ui_widget.vert.spv / ui_widget.frag.spv
};
struct GpuRendererOptions {
    std::size_t layers{3};               // 1..8 layer textures (see GpuFrameOptions::max_layers)
    std::uint32_t width{}, height{};     // initial layer size; grows to the viewport on demand
};
// GPU backend: draws the runtime's placed result with Pictor's UIRenderer (quads, scissors and
// layer textures). Text and shapes the quad pipeline lacks are rasterized by Pictor's TrueType
// rasterizer and Tela's shape raster into the layers, so text and shapes look as on the CPU surface.
class GpuRenderer {
public:
    GpuRenderer(const GpuTarget&, const std::string& true_type_font, const GpuRendererOptions& = {});
    ~GpuRenderer();
    GpuRenderer(const GpuRenderer&) = delete;
    GpuRenderer& operator=(const GpuRenderer&) = delete;
    // Outside any render pass: plans the frame and uploads the layers that changed. Returns
    // whether the plan differs from the previous one. Does not mark the runtime presented.
    bool prepare(const Runtime&);
    // Inside the host's render pass: records the prepared frame.
    void record(VkCommandBuffer, VkExtent2D);
    // Advances of the loaded font, for Runtime::text_measure. Valid while this renderer lives.
    TextMeasure measure() const;
    const GpuFrame& frame() const noexcept;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
