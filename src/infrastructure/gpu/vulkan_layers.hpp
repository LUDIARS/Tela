#pragma once
// @implements SPEC-TL-GPU
// @spec GPU rendering
#include <tela/pixel_surface.hpp>
#include <cstdint>
#include <vector>
#include <vulkan/vulkan.h>

namespace pictor { class VulkanContext; }

namespace tela::gpu {
// Sampled RGBA textures the layers are uploaded to, each `width` x `height` with its own mapped
// staging buffer. Owned here; the quad pipeline only borrows the views.
class VulkanLayers {
public:
    VulkanLayers() = default;
    ~VulkanLayers();
    VulkanLayers(const VulkanLayers&) = delete;
    VulkanLayers& operator=(const VulkanLayers&) = delete;
    // Replaces any earlier textures. Throws std::runtime_error when Vulkan refuses.
    void create(pictor::VulkanContext&, std::size_t count, std::uint32_t width, std::uint32_t height, VkFormat);
    void destroy();
    // Copies the surface (premultiplied BGRA, at most width x height) into texture `index`
    // and waits for the copy, so a frame never samples half an upload.
    void upload(std::size_t index, const PixelSurface&);
    VkImageView view(std::size_t index) const { return layers_[index].view; }
    std::size_t count() const noexcept { return layers_.size(); }
    std::uint32_t width() const noexcept { return width_; }
    std::uint32_t height() const noexcept { return height_; }
private:
    struct Layer {
        VkImage image{VK_NULL_HANDLE};
        VkDeviceMemory memory{VK_NULL_HANDLE};
        VkImageView view{VK_NULL_HANDLE};
        VkBuffer staging{VK_NULL_HANDLE};
        VkDeviceMemory staging_memory{VK_NULL_HANDLE};
        void* mapped{};
        bool uploaded{};
    };
    Layer make(VkFormat) const;
    std::uint32_t memory_type(std::uint32_t filter, VkMemoryPropertyFlags) const;
    pictor::VulkanContext* vulkan_{};
    std::vector<Layer> layers_;
    std::uint32_t width_{}, height_{};
};
}
