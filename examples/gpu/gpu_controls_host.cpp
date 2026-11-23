// @implements SPEC-TL-GPU
// @spec GPU rendering
// GPU check host: the control gallery in a GLFW + Vulkan window drawn by Tela::Gpu.
// tela_gpu_controls --font <file.ttf> [--shaders <dir>] [--seconds N]
#include "../controls/controls_gallery.hpp"
#include "glfw_input.hpp"
#include <tela/gpu_renderer.hpp>
#include <pictor/surface/glfw_surface_provider.h>
#include <pictor/surface/vulkan_context.h>
#include <GLFW/glfw3.h>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <string>

#ifndef TELA_GPU_SHADER_DIR
#define TELA_GPU_SHADER_DIR "shaders"
#endif

namespace {
struct Options { std::string font, shaders{TELA_GPU_SHADER_DIR}; int seconds{}; };
Options parse(int argc, char** argv) {
    Options options;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (i + 1 >= argc) throw std::invalid_argument("missing value for " + arg);
        if (arg == "--font") options.font = argv[++i];
        else if (arg == "--shaders") options.shaders = argv[++i];
        else if (arg == "--seconds") options.seconds = std::stoi(argv[++i]);
        else throw std::invalid_argument("unknown option " + arg);
    }
    if (options.font.empty())
        throw std::invalid_argument("usage: tela_gpu_controls --font <file.ttf> [--shaders <dir>] [--seconds N]");
    if (options.seconds < 0 || options.seconds > 86400) throw std::invalid_argument("--seconds must be 0..86400");
    return options;
}
// Keeps the runtime's viewport equal to the window: framebuffer pixels, content scale as DPI,
// visibility (not minimized) and focus. Returns true when something changed.
bool synchronize(GLFWwindow* window, tela::Runtime& runtime, GlfwInput& input) {
    int fb_w = 0, fb_h = 0, win_w = 0, win_h = 0;
    float scale_x = 1, scale_y = 1;
    glfwGetFramebufferSize(window, &fb_w, &fb_h);
    glfwGetWindowSize(window, &win_w, &win_h);
    glfwGetWindowContentScale(window, &scale_x, &scale_y);
    input.pixel_ratio(win_w ? static_cast<float>(fb_w) / win_w : 1, win_h ? static_cast<float>(fb_h) / win_h : 1);
    tela::Viewport next = runtime.viewport();
    next.host_id = "glfw";
    next.view_id = "controls";
    next.width = fb_w;
    next.height = fb_h;
    next.dpi_scale = scale_x > 0 ? scale_x : 1;
    next.visible = fb_w > 0 && fb_h > 0 && !glfwGetWindowAttrib(window, GLFW_ICONIFIED);
    next.focused = glfwGetWindowAttrib(window, GLFW_FOCUSED) != 0;
    const auto& now = runtime.viewport();
    if (next.host_id == now.host_id && next.view_id == now.view_id && next.width == now.width &&
        next.height == now.height && next.dpi_scale == now.dpi_scale && next.visible == now.visible &&
        next.focused == now.focused)
        return false;
    ++next.revision;
    runtime.viewport(std::move(next));
    return true;
}
// Records and presents one frame. Returns false when the swapchain had no image to draw into.
bool present(pictor::VulkanContext& vk, tela::GpuRenderer& renderer) {
    const auto frame = vk.acquire_frame();
    if (!frame.has_image()) {
        if (pictor::frame_status_requires_reinitialize(frame.status)) throw std::runtime_error("Vulkan surface or device lost");
        return false;
    }
    const VkCommandBuffer cmd = vk.command_buffers()[frame.image_index];
    vkResetCommandBuffer(cmd, 0);
    VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    vkBeginCommandBuffer(cmd, &begin);
    const VkExtent2D extent = vk.swapchain_extent();
    const VkClearValue clear{{{0, 0, 0, 1}}};
    VkRenderPassBeginInfo pass{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    pass.renderPass = vk.default_render_pass();
    pass.framebuffer = vk.framebuffers()[frame.image_index];
    pass.renderArea = {{0, 0}, extent};
    pass.clearValueCount = 1;
    pass.pClearValues = &clear;
    vkCmdBeginRenderPass(cmd, &pass, VK_SUBPASS_CONTENTS_INLINE);
    renderer.record(cmd, extent);
    vkCmdEndRenderPass(cmd);
    vkEndCommandBuffer(cmd);
    const VkPipelineStageFlags stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    const VkSemaphore wait = vk.image_available_semaphore(), signal = vk.render_finished_semaphore();
    VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit.waitSemaphoreCount = 1;
    submit.pWaitSemaphores = &wait;
    submit.pWaitDstStageMask = &stage;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &cmd;
    submit.signalSemaphoreCount = 1;
    submit.pSignalSemaphores = &signal;
    if (vkQueueSubmit(vk.graphics_queue(), 1, &submit, vk.in_flight_fence()) != VK_SUCCESS)
        throw std::runtime_error("vkQueueSubmit failed");
    return vk.present_frame(frame.image_index).status != pictor::FrameStatus::RecreateSwapchain;
}
}

int main(int argc, char** argv) {
    try {
        const auto options = parse(argc, argv);
        pictor::GlfwSurfaceProvider window;
        if (!window.create({520, 720, "Tela GPU controls", true, true})) throw std::runtime_error("GLFW window creation failed");
        pictor::VulkanContext vk;
        if (!vk.initialize(&window)) throw std::runtime_error("Vulkan initialization failed: " + vk.init_result().detail);
        {
            tela::Runtime runtime;
            tela::GpuRenderer renderer({vk, vk.default_render_pass(), vk.swapchain_format(), options.shaders}, options.font);
            runtime.text_measure(renderer.measure());
            GlfwInput input(window.glfw_window(), runtime);
            ControlsGallery gallery(runtime);
            const auto started = std::chrono::steady_clock::now();
            bool redraw = true; // the swapchain needs an image even when the runtime did not change
            while (!window.should_close()) {
                if (options.seconds && std::chrono::steady_clock::now() - started >= std::chrono::seconds(options.seconds)) break;
                // Sleep until input arrives; a timeout still notices focus and size changes.
                glfwWaitEventsTimeout(0.25);
                redraw = synchronize(window.glfw_window(), runtime, input) || redraw;
                if (!runtime.viewport().visible || (!runtime.needs_frame() && !redraw)) continue;
                renderer.prepare(runtime);
                redraw = !present(vk, renderer);
                if (!redraw) runtime.frame_presented();
            }
            vk.device_wait_idle();
        }
        vk.shutdown();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Tela GPU controls: " << error.what() << '\n';
        return 2;
    }
}
