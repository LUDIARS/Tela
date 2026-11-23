// @spec SPEC-TL-GPU
#include "glfw_input.hpp"
#include <GLFW/glfw3.h>
#include <cmath>
#include <string>

namespace {
GlfwInput* attached = nullptr;

std::uint32_t modifiers(int mods) {
    std::uint32_t bits = 0;
    if (mods & GLFW_MOD_SHIFT) bits |= tela::modifier_shift;
    if (mods & GLFW_MOD_CONTROL) bits |= tela::modifier_control;
    if (mods & GLFW_MOD_ALT) bits |= tela::modifier_alt;
    if (mods & GLFW_MOD_SUPER) bits |= tela::modifier_meta;
    return bits;
}
tela::Key translate(int key) {
    switch (key) {
    case GLFW_KEY_TAB: return tela::Key::tab;
    case GLFW_KEY_ENTER: case GLFW_KEY_KP_ENTER: return tela::Key::enter;
    case GLFW_KEY_ESCAPE: return tela::Key::escape;
    case GLFW_KEY_BACKSPACE: return tela::Key::backspace;
    case GLFW_KEY_DELETE: return tela::Key::delete_forward;
    case GLFW_KEY_LEFT: return tela::Key::left;
    case GLFW_KEY_RIGHT: return tela::Key::right;
    case GLFW_KEY_UP: return tela::Key::up;
    case GLFW_KEY_DOWN: return tela::Key::down;
    case GLFW_KEY_HOME: return tela::Key::home;
    case GLFW_KEY_END: return tela::Key::end;
    case GLFW_KEY_PAGE_UP: return tela::Key::page_up;
    case GLFW_KEY_PAGE_DOWN: return tela::Key::page_down;
    case GLFW_KEY_A: return tela::Key::a;
    default: return tela::Key::none;
    }
}
std::string utf8(unsigned int cp) {
    std::string out;
    if (cp < 0x80) out.push_back(static_cast<char>(cp));
    else if (cp < 0x800) {
        out.push_back(static_cast<char>(0xC0 | (cp >> 6)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else if (cp < 0x10000) {
        out.push_back(static_cast<char>(0xE0 | (cp >> 12)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else {
        out.push_back(static_cast<char>(0xF0 | (cp >> 18)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    }
    return out;
}
}

GlfwInput::GlfwInput(GLFWwindow* window, tela::Runtime& runtime) : window_(window), runtime_(runtime) {
    attached = this;
    glfwSetCursorPosCallback(window, on_cursor);
    glfwSetMouseButtonCallback(window, on_button);
    glfwSetScrollCallback(window, on_scroll);
    glfwSetKeyCallback(window, on_key);
    glfwSetCharCallback(window, on_char);
}
GlfwInput::~GlfwInput() {
    glfwSetCursorPosCallback(window_, nullptr);
    glfwSetMouseButtonCallback(window_, nullptr);
    glfwSetScrollCallback(window_, nullptr);
    glfwSetKeyCallback(window_, nullptr);
    glfwSetCharCallback(window_, nullptr);
    if (attached == this) attached = nullptr;
}
void GlfwInput::pointer(tela::PointerPhase phase, tela::PointerButton button, float wheel_x, float wheel_y) {
    tela::HostPointerEvent event;
    event.sequence = ++pointer_sequence_;
    event.viewport_revision = runtime_.viewport().revision;
    event.gesture_id = gesture_;
    event.phase = phase;
    event.button = button;
    // The viewport starts at desktop (0, 0), so framebuffer pixels are the desktop pixels.
    event.desktop_x = static_cast<int>(std::lround(x_ * ratio_x_));
    event.desktop_y = static_cast<int>(std::lround(y_ * ratio_y_));
    event.wheel_x = wheel_x;
    event.wheel_y = wheel_y;
    event.modifiers = modifiers_;
    runtime_.pointer(event, tela::InputSource::native);
}
void GlfwInput::on_cursor(GLFWwindow*, double x, double y) {
    if (!attached) return;
    attached->x_ = x;
    attached->y_ = y;
    attached->pointer(tela::PointerPhase::move, tela::PointerButton::none);
}
void GlfwInput::on_button(GLFWwindow*, int button, int action, int mods) {
    if (!attached || button != GLFW_MOUSE_BUTTON_LEFT) return;
    attached->modifiers_ = modifiers(mods);
    if (action == GLFW_PRESS) ++attached->gesture_;
    attached->pointer(action == GLFW_PRESS ? tela::PointerPhase::down : tela::PointerPhase::up, tela::PointerButton::primary);
}
void GlfwInput::on_scroll(GLFWwindow*, double x, double y) {
    // GLFW's positive y offset scrolls away from the user, as the contract's wheel_y does.
    if (attached) attached->pointer(tela::PointerPhase::wheel, tela::PointerButton::none, static_cast<float>(x), static_cast<float>(y));
}
void GlfwInput::on_key(GLFWwindow*, int key, int, int action, int mods) {
    if (!attached) return;
    attached->modifiers_ = modifiers(mods);
    if (action == GLFW_RELEASE) return;
    const auto translated = translate(key);
    if (translated == tela::Key::none) return;
    attached->runtime_.key({++attached->key_sequence_, translated, true, attached->modifiers_});
}
void GlfwInput::on_char(GLFWwindow*, unsigned int codepoint) {
    // Committed characters only (GLFW delivers an IME's result here, not its composition).
    if (!attached || codepoint < 0x20 || codepoint == 0x7F || (codepoint >= 0xD800 && codepoint <= 0xDFFF) || codepoint > 0x10FFFF)
        return;
    attached->runtime_.text({++attached->key_sequence_, utf8(codepoint)});
}
