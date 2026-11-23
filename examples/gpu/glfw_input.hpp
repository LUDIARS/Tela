#pragma once
// @implements SPEC-TL-GPU
// @spec GPU rendering
#include <tela/runtime.hpp>
#include <cstdint>

struct GLFWwindow;

// Turns one GLFW window's mouse, wheel, keys and committed characters into runtime input. The
// window's user pointer belongs to Pictor's surface provider, so the callbacks reach this object
// through a single process-wide slot: one attached window per process (this is a check host).
class GlfwInput {
public:
    GlfwInput(GLFWwindow*, tela::Runtime&);
    ~GlfwInput();
    GlfwInput(const GlfwInput&) = delete;
    GlfwInput& operator=(const GlfwInput&) = delete;
    // Framebuffer pixels per window coordinate; GLFW reports the cursor in window coordinates.
    void pixel_ratio(float x, float y) noexcept { ratio_x_ = x; ratio_y_ = y; }
private:
    static void on_cursor(GLFWwindow*, double x, double y);
    static void on_button(GLFWwindow*, int button, int action, int mods);
    static void on_scroll(GLFWwindow*, double x, double y);
    static void on_key(GLFWwindow*, int key, int scancode, int action, int mods);
    static void on_char(GLFWwindow*, unsigned int codepoint);
    void pointer(tela::PointerPhase, tela::PointerButton, float wheel_x = 0, float wheel_y = 0);
    GLFWwindow* window_;
    tela::Runtime& runtime_;
    double x_{}, y_{};
    float ratio_x_{1}, ratio_y_{1};
    std::uint32_t modifiers_{};
    std::uint64_t pointer_sequence_{}, gesture_{}, key_sequence_{};
};
