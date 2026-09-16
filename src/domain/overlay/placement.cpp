// @spec SPEC-TL-PLACEMENT
#include <tela/placement.hpp>
#include <algorithm>
#include <array>
#include <stdexcept>
#include <utility>
#include <cmath>

namespace tela {
namespace {
bool is_usable(float value) { return std::isfinite(value) && value > 0; }
}

Placement placement_from_name(std::string_view name) {
    constexpr std::array<std::pair<std::string_view, Placement>, 5> names{{
        {"inside", Placement::inside}, {"left", Placement::left}, {"right", Placement::right},
        {"above", Placement::above}, {"below", Placement::below}}};
    for(const auto& [text, value] : names) if(name == text) return value;
    throw std::invalid_argument("Placement must be inside, left, right, above or below");
}

Rect place_viewport(Placement placement, Rect host, float width, float height, Rect work_area) {
    if(placement == Placement::inside) return host;
    if(!is_usable(width) || !is_usable(height) || !is_usable(work_area.width) || !is_usable(work_area.height)) return host;
    const float w = std::min(width, work_area.width);
    const float h = std::min(height, work_area.height);
    const float right_edge = work_area.x + work_area.width, bottom_edge = work_area.y + work_area.height;
    float x = host.x, y = host.y;
    switch(placement) {
    case Placement::right: x = host.x + host.width; break;
    case Placement::left:  x = host.x - w; break;
    case Placement::above: y = host.y - h; break;
    case Placement::below: y = host.y + host.height; break;
    case Placement::inside: break;
    }
    // A side without room flips to the opposite one rather than covering the host.
    if(placement == Placement::right && x + w > right_edge) x = host.x - w;
    else if(placement == Placement::left && x < work_area.x) x = host.x + host.width;
    else if(placement == Placement::below && y + h > bottom_edge) y = host.y - h;
    else if(placement == Placement::above && y < work_area.y) y = host.y + host.height;
    // Neither side fitting still has to stay on screen; overlap is better than being invisible.
    x = std::clamp(x, work_area.x, right_edge - w);
    y = std::clamp(y, work_area.y, bottom_edge - h);
    return {x, y, w, h};
}
}
