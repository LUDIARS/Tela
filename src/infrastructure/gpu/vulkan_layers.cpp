// @spec SPEC-TL-GPU
#include "vulkan_layers.hpp"
#include <tela/gpu_frame.hpp>
#include <pictor/surface/vulkan_context.h>
#include <stdexcept>
#include <string>

namespace tela::gpu {
namespace {
void check(VkResult result, const char* what) {
    if (result != VK_SUCCESS) throw std::runtime_error(std::string("Tela GPU layer: ") + what + " failed");
}
void barrier(VkCommandBuffer cmd, VkImage image, VkImageLayout from, VkImageLayout to, VkAccessFlags src,
             VkAccessFlags dst, VkPipelineStageFlags src_stage, VkPipelineStageFlags dst_stage) {
    VkImageMemoryBarrier b{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    b.oldLayout = from;
    b.newLayout = to;
    b.srcAccessMask = src;
    b.dstAccessMask = dst;
    b.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    b.image = image;
    b.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    vkCmdPipelineBarrier(cmd, src_stage, dst_stage, 0, 0, nullptr, 0, nullptr, 1, &b);
}
}
VulkanLayers::~VulkanLayers() { destroy(); }

std::uint32_t VulkanLayers::memory_type(std::uint32_t filter, VkMemoryPropertyFlags flags) const {
    VkPhysicalDeviceMemoryProperties properties{};
    vkGetPhysicalDeviceMemoryProperties(vulkan_->physical_device(), &properties);
    for (std::uint32_t i = 0; i < properties.memoryTypeCount; ++i)
        if ((filter & (1u << i)) && (properties.memoryTypes[i].propertyFlags & flags) == flags) return i;
    throw std::runtime_error("Tela GPU layer: no suitable memory type");
}
VulkanLayers::Layer VulkanLayers::make(VkFormat format) const {
    const VkDevice device = vulkan_->device();
    Layer layer;
    VkImageCreateInfo image{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    image.imageType = VK_IMAGE_TYPE_2D;
    image.format = format;
    image.extent = {width_, height_, 1};
    image.mipLevels = 1;
    image.arrayLayers = 1;
    image.samples = VK_SAMPLE_COUNT_1_BIT;
    image.tiling = VK_IMAGE_TILING_OPTIMAL;
    image.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    image.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    check(vkCreateImage(device, &image, nullptr, &layer.image), "vkCreateImage");
    VkMemoryRequirements requirements{};
    vkGetImageMemoryRequirements(device, layer.image, &requirements);
    VkMemoryAllocateInfo allocate{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    allocate.allocationSize = requirements.size;
    allocate.memoryTypeIndex = memory_type(requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    check(vkAllocateMemory(device, &allocate, nullptr, &layer.memory), "image memory");
    check(vkBindImageMemory(device, layer.image, layer.memory, 0), "vkBindImageMemory");
    VkImageViewCreateInfo view{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    view.image = layer.image;
    view.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view.format = format;
    view.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    check(vkCreateImageView(device, &view, nullptr, &layer.view), "vkCreateImageView");
    VkBufferCreateInfo buffer{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    buffer.size = static_cast<VkDeviceSize>(width_) * height_ * 4;
    buffer.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    buffer.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    check(vkCreateBuffer(device, &buffer, nullptr, &layer.staging), "staging buffer");
    vkGetBufferMemoryRequirements(device, layer.staging, &requirements);
    allocate.allocationSize = requirements.size;
    allocate.memoryTypeIndex = memory_type(requirements.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    check(vkAllocateMemory(device, &allocate, nullptr, &layer.staging_memory), "staging memory");
    check(vkBindBufferMemory(device, layer.staging, layer.staging_memory, 0), "vkBindBufferMemory");
    check(vkMapMemory(device, layer.staging_memory, 0, buffer.size, 0, &layer.mapped), "vkMapMemory");
    return layer;
}
void VulkanLayers::create(pictor::VulkanContext& vulkan, std::size_t count, std::uint32_t width, std::uint32_t height,
                          VkFormat format) {
    destroy();
    vulkan_ = &vulkan;
    width_ = width;
    height_ = height;
    try {
        for (std::size_t i = 0; i < count; ++i) {
            layers_.emplace_back();
            layers_.back() = make(format);
        }
    } catch (...) {
        destroy();
        throw;
    }
}
void VulkanLayers::destroy() {
    if (!vulkan_) return;
    const VkDevice device = vulkan_->device();
    vulkan_->device_wait_idle();
    for (auto& layer : layers_) {
        if (layer.mapped) vkUnmapMemory(device, layer.staging_memory);
        if (layer.staging) vkDestroyBuffer(device, layer.staging, nullptr);
        if (layer.staging_memory) vkFreeMemory(device, layer.staging_memory, nullptr);
        if (layer.view) vkDestroyImageView(device, layer.view, nullptr);
        if (layer.image) vkDestroyImage(device, layer.image, nullptr);
        if (layer.memory) vkFreeMemory(device, layer.memory, nullptr);
    }
    layers_.clear();
    vulkan_ = nullptr;
    width_ = height_ = 0;
}
void VulkanLayers::upload(std::size_t index, const PixelSurface& surface) {
    auto& layer = layers_.at(index);
    if (surface.width <= 0 || surface.height <= 0) return;
    if (static_cast<std::uint32_t>(surface.width) > width_ || static_cast<std::uint32_t>(surface.height) > height_)
        throw std::length_error("Tela GPU layer: surface larger than its texture");
    layer_texels(surface, static_cast<unsigned char*>(layer.mapped), static_cast<int>(width_));
    VkCommandBuffer cmd = vulkan_->begin_single_time_commands();
    barrier(cmd, layer.image, layer.uploaded ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, layer.uploaded ? VK_ACCESS_SHADER_READ_BIT : 0,
            VK_ACCESS_TRANSFER_WRITE_BIT,
            layer.uploaded ? VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT : VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT);
    VkBufferImageCopy region{};
    region.bufferRowLength = width_;
    region.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    region.imageExtent = {static_cast<std::uint32_t>(surface.width), static_cast<std::uint32_t>(surface.height), 1};
    vkCmdCopyBufferToImage(cmd, layer.staging, layer.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    barrier(cmd, layer.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    vulkan_->end_single_time_commands(cmd); // waits for the queue
    layer.uploaded = true;
}
}
