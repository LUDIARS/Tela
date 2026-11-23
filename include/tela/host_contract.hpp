#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace tela {
// Host adapters convert all geometry to desktop physical pixels, including DPI.
struct Viewport {
    std::string host_id;
    std::string view_id;
    std::uint64_t revision{};
    int desktop_x{}, desktop_y{}, width{}, height{};
    float dpi_scale{1.0f};
    bool visible{}, focused{};
};
// wheel_x / wheel_y are notches; positive wheel_y turns away from the user (content moves toward its start).
enum class PointerPhase { move, down, up, cancel, wheel };
enum class PointerButton { none, primary, secondary, middle };
// Observation of host-owned input, never an instruction to inject another click.
struct HostPointerEvent {
    std::uint64_t sequence{}, viewport_revision{}, gesture_id{};
    PointerPhase phase{};
    PointerButton button{};
    int desktop_x{}, desktop_y{};
    float wheel_x{}, wheel_y{};
    std::uint32_t modifiers{};
};
// Modifier bits shared by pointer and key events.
constexpr std::uint32_t modifier_shift = 1, modifier_control = 2, modifier_alt = 4, modifier_meta = 8;
// Keys the runtime interprets. Printable characters arrive as HostTextEvent instead, so
// letters appear here only for shortcuts (`a` with control/meta selects all).
enum class Key { none, tab, enter, escape, backspace, delete_forward, left, right, up, down,
                 home, end, page_up, page_down, a };
struct HostKeyEvent {
    std::uint64_t sequence{};
    Key key{};
    bool pressed{}; // a repeat is another pressed event
    std::uint32_t modifiers{};
};
// Committed text: typed characters or an IME's confirmed string, never a composition preview.
struct HostTextEvent {
    std::uint64_t sequence{};
    std::string text;
};
struct HostSelection {
    std::uint64_t sequence{};
    std::vector<std::string> object_ids;
};
} // namespace tela
