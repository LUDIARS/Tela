// @spec SPEC-TL-CONTROLS
#include "native_keyboard.hpp"
#include <string>

namespace tela::windows {
namespace {
Key translate(WPARAM vk) noexcept {
    switch (vk) {
    case VK_TAB: return Key::tab;
    case VK_RETURN: return Key::enter;
    case VK_ESCAPE: return Key::escape;
    case VK_BACK: return Key::backspace;
    case VK_DELETE: return Key::delete_forward;
    case VK_LEFT: return Key::left;
    case VK_RIGHT: return Key::right;
    case VK_UP: return Key::up;
    case VK_DOWN: return Key::down;
    case VK_HOME: return Key::home;
    case VK_END: return Key::end;
    case VK_PRIOR: return Key::page_up;
    case VK_NEXT: return Key::page_down;
    case 'A': return Key::a;
    default: return Key::none;
    }
}
std::string utf8(const wchar_t* units, int count) {
    const int size = WideCharToMultiByte(CP_UTF8, 0, units, count, nullptr, 0, nullptr, nullptr);
    std::string text(static_cast<std::size_t>(size > 0 ? size : 0), '\0');
    if (size > 0) WideCharToMultiByte(CP_UTF8, 0, units, count, text.data(), size, nullptr, nullptr);
    return text;
}
}
std::uint32_t current_modifiers() noexcept {
    std::uint32_t bits = 0;
    if (GetKeyState(VK_SHIFT) < 0) bits |= modifier_shift;
    if (GetKeyState(VK_CONTROL) < 0) bits |= modifier_control;
    if (GetKeyState(VK_MENU) < 0) bits |= modifier_alt;
    if (GetKeyState(VK_LWIN) < 0 || GetKeyState(VK_RWIN) < 0) bits |= modifier_meta;
    return bits;
}
bool NativeKeyboard::key(Runtime& runtime, WPARAM virtual_key) {
    const auto key = translate(virtual_key);
    if (key == Key::none) return false;
    return runtime.key({++sequence_, key, true, current_modifiers()});
}
bool NativeKeyboard::character(Runtime& runtime, WPARAM code_unit) {
    const auto unit = static_cast<wchar_t>(code_unit);
    if (unit >= 0xD800 && unit <= 0xDBFF) { high_surrogate_ = unit; return true; }
    std::string text;
    if (unit >= 0xDC00 && unit <= 0xDFFF) {
        if (!high_surrogate_) return false;
        const wchar_t pair[2]{high_surrogate_, unit};
        text = utf8(pair, 2);
    } else {
        // Control characters (Enter, Tab, Backspace, Ctrl+letter) already arrived as keys.
        if (unit < 0x20 || unit == 0x7F) { high_surrogate_ = 0; return false; }
        text = utf8(&unit, 1);
    }
    high_surrogate_ = 0;
    return runtime.text({++sequence_, std::move(text)});
}
}
