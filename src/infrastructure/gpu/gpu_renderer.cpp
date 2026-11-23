// @spec SPEC-TL-GPU
#include <tela/gpu_renderer.hpp>
#include "layer_raster.hpp"
#include "ui_commands.hpp"
#include "vulkan_layers.hpp"
#include <pictor/surface/vulkan_context.h>
#include <pictor/text/text_image_renderer.h>
#include <pictor/ui/ui_renderer.h>
#include <algorithm>
#include <stdexcept>

namespace tela {
namespace {
constexpr std::size_t layer_limit = 8; // of UIRenderer's 64 texture slots, which are never released
bool srgb(VkFormat format) {
    return format == VK_FORMAT_B8G8R8A8_SRGB || format == VK_FORMAT_R8G8B8A8_SRGB ||
           format == VK_FORMAT_A8B8G8R8_SRGB_PACK32;
}
}
struct GpuRenderer::Impl {
    pictor::FontLoader fonts;
    pictor::TextImageRenderer text{fonts};
    pictor::FontHandle font;
    pictor_adapter::PictorText line;
    GpuTarget target;
    std::size_t layer_count;
    gpu::VulkanLayers layers;      // destroyed after the renderer that samples them
    pictor::UIRenderer renderer;
    std::vector<std::uint32_t> textures;
    GpuFrame frame;
    std::vector<GpuLayer> uploaded; // what each texture holds
    int uploaded_width{}, uploaded_height{};
    PixelSurface scratch;

    Impl(const GpuTarget& t, const std::string& path, const GpuRendererOptions& options)
        : font(pictor_adapter::load_true_type(fonts, path)), line(text, font), target(t),
          layer_count(std::clamp<std::size_t>(options.layers, 1, layer_limit)) {
        if (target.render_pass == VK_NULL_HANDLE) throw std::invalid_argument("Tela GPU: a render pass is required");
        start(std::max(1u, options.width), std::max(1u, options.height));
    }
    // UIRenderer keeps a descriptor for every registered view and cannot drop one, so new layer
    // textures come with a new UIRenderer.
    void start(std::uint32_t width, std::uint32_t height) {
        renderer.shutdown();
        textures.clear();
        uploaded.clear();
        layers.create(target.vulkan, layer_count, width, height,
                      srgb(target.format) ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM);
        if (!renderer.initialize(target.vulkan, target.shader_dir, target.render_pass))
            throw std::runtime_error("Tela GPU: Pictor UIRenderer did not start (shaders in " + target.shader_dir + "?)");
        for (std::size_t i = 0; i < layers.count(); ++i) {
            const auto id = renderer.register_texture(layers.view(i));
            if (!id) throw std::runtime_error("Tela GPU: Pictor UIRenderer refused a layer texture");
            textures.push_back(id);
        }
    }
    void fit(int width, int height) {
        const auto w = static_cast<std::uint32_t>(std::max(1, width)), h = static_cast<std::uint32_t>(std::max(1, height));
        if (w <= layers.width() && h <= layers.height()) return;
        start(std::max(w, layers.width()), std::max(h, layers.height()));
    }
    void upload(const GpuFrame& next) {
        const bool resized = next.width != uploaded_width || next.height != uploaded_height;
        uploaded.resize(std::max(uploaded.size(), next.layers.size()));
        for (std::size_t i = 0; i < next.layers.size(); ++i) {
            if (!resized && uploaded[i] == next.layers[i]) continue;
            scratch.width = next.width;
            scratch.height = next.height;
            scratch.pixels.resize(static_cast<std::size_t>(next.width) * next.height * 4);
            gpu::raster_layer(scratch, next.layers[i], line);
            layers.upload(i, scratch);
            uploaded[i] = next.layers[i];
        }
        uploaded_width = next.width;
        uploaded_height = next.height;
    }
};
GpuRenderer::GpuRenderer(const GpuTarget& target, const std::string& font, const GpuRendererOptions& options)
    : impl_(std::make_unique<Impl>(target, font, options)) {}
GpuRenderer::~GpuRenderer() = default;
bool GpuRenderer::prepare(const Runtime& runtime) {
    return prepare(runtime, GpuQuad{});
}
bool GpuRenderer::prepare(const Runtime& runtime, const GpuQuad& overlay) {
    auto next = plan_gpu_frame(runtime, {impl_->layer_count});
    if (overlay.color.a && overlay.rect.width > 0 && overlay.rect.height > 0)
        next.steps.emplace_back(overlay);
    const bool changed = !(next == impl_->frame);
    if (next.width > 0 && next.height > 0) {
        impl_->fit(next.width, next.height);
        impl_->upload(next);
    }
    impl_->frame = std::move(next);
    return changed;
}
void GpuRenderer::record(VkCommandBuffer cmd, VkExtent2D extent) {
    if (impl_->frame.steps.empty()) return;
    const gpu::LayerTextures textures{impl_->textures, static_cast<float>(impl_->layers.width()),
                                      static_cast<float>(impl_->layers.height())};
    impl_->renderer.record(cmd, extent, gpu::ui_commands(impl_->frame, textures, srgb(impl_->target.format)));
}
TextMeasure GpuRenderer::measure() const {
    const auto* line = &impl_->line;
    return [line](std::string_view text, float font_size) { return line->width(text, font_size); };
}
const GpuFrame& GpuRenderer::frame() const noexcept { return impl_->frame; }
}
