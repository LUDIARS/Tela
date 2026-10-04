// @spec SPEC-TL-DESKTOP-OVERLAY
#include <tela/desktop_points.hpp>
#include <cmath>
#include <stdexcept>

namespace tela {
namespace {
constexpr float min_backing_scale = .25f, max_backing_scale = 8;
constexpr double max_coordinate = 1000000;

bool finite_within(double value) { return std::isfinite(value) && std::abs(value) <= max_coordinate; }

bool usable(const PointDisplay& display, double primary_top) {
    return finite_within(display.x) && finite_within(display.y) && finite_within(display.width)
        && finite_within(display.height) && display.width > 0 && display.height > 0
        && finite_within(primary_top) && std::isfinite(display.backing_scale)
        && display.backing_scale >= min_backing_scale && display.backing_scale <= max_backing_scale;
}

int point(double value) { return static_cast<int>(std::lround(value)); }

// Edges are rounded rather than the origin and size, so neighbouring work areas still meet.
DesktopMonitor monitor_of(const PointDisplay& display, double primary_top) {
    const int left = point(display.x), right = point(display.x + display.width);
    const int top = point(primary_top - display.y - display.height), bottom = point(primary_top - display.y);
    return {left, top, right - left, bottom - top, 1, display.primary};
}

struct Usable { std::vector<DesktopMonitor> monitors; std::vector<float> scales; };

Usable usable_monitors(const std::vector<PointDisplay>& displays, double primary_top) {
    Usable result;
    for(const auto& display : displays) {
        if(!usable(display, primary_top)) continue;
        const auto monitor = monitor_of(display, primary_top);
        // A display narrower than half a point rounds to nothing and holds no surface.
        if(monitor.width <= 0 || monitor.height <= 0) continue;
        result.monitors.push_back(monitor);
        result.scales.push_back(display.backing_scale);
    }
    return result;
}

bool holds(const DesktopMonitor& monitor, const DesktopFrame& frame) {
    return frame.x >= monitor.x && frame.y >= monitor.y && frame.x + frame.width <= monitor.x + monitor.width
        && frame.y + frame.height <= monitor.y + monitor.height;
}

// place_on_desktop always settles inside one work area; find it, preferring the primary when
// mirrored displays share the same area, so the surface is drawn at that display's scale.
float scale_holding(const Usable& usable, const DesktopFrame& frame) {
    float scale = 0;
    for(std::size_t i = 0; i < usable.monitors.size(); ++i) {
        if(!holds(usable.monitors[i], frame)) continue;
        if(usable.monitors[i].primary) return usable.scales[i];
        if(!scale) scale = usable.scales[i];
    }
    if(!scale) throw std::logic_error("A placed desktop surface lies on no display");
    return scale;
}
}

std::vector<DesktopMonitor> point_monitors(const std::vector<PointDisplay>& displays, double primary_top) {
    return usable_monitors(displays, primary_top).monitors;
}

PointFrame place_on_point_desktop(const DesktopPlacement& placement, const std::vector<PointDisplay>& displays,
                                  double primary_top) {
    const auto usable = usable_monitors(displays, primary_top);
    PointFrame result;
    result.points = place_on_desktop(placement, usable.monitors);
    const float scale = scale_holding(usable, result.points);
    result.points.dpi_scale = scale;
    const auto& frame = result.points;
    result.appkit_x = frame.x;
    result.appkit_y = primary_top - frame.y - frame.height;
    result.pixel_x = point(static_cast<double>(frame.x) * scale);
    result.pixel_y = point(static_cast<double>(frame.y) * scale);
    result.pixel_width = point(static_cast<double>(frame.width) * scale);
    result.pixel_height = point(static_cast<double>(frame.height) * scale);
    return result;
}

DesktopPlacement point_placement_at(const DesktopPlacement& current, int x, int y,
                                    const std::vector<PointDisplay>& displays, double primary_top) {
    return placement_at(current, x, y, point_monitors(displays, primary_top));
}
}
