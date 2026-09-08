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
struct HostSelection {
    std::uint64_t sequence{};
    std::vector<std::string> object_ids;
};
} // namespace tela
