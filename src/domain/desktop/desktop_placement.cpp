// @spec SPEC-TL-DESKTOP-OVERLAY
#include <tela/desktop_placement.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <stdexcept>
#include <utility>

namespace tela {
namespace {
// The Runtime accepts no viewport outside these limits, so a surface or monitor beyond them
// cannot be presented at all.
constexpr float min_dpi_scale = .25f, max_dpi_scale = 8;
constexpr float max_logical_size = 16384;
constexpr float max_margin = 100000;
constexpr int max_coordinate = 1000000;
// One table for both directions so a stored name always reads back as the same corner.
constexpr std::array<std::pair<std::string_view, DesktopCorner>, 4> corner_names{{
    {"top-left", DesktopCorner::top_left}, {"top-right", DesktopCorner::top_right},
    {"bottom-left", DesktopCorner::bottom_left}, {"bottom-right", DesktopCorner::bottom_right}}};

bool within(float value, float low, float high) { return std::isfinite(value) && value >= low && value <= high; }

void validate(const DesktopPlacement& placement) {
    if(!within(placement.width, 0, max_logical_size) || !within(placement.height, 0, max_logical_size)
       || placement.width <= 0 || placement.height <= 0)
        throw std::invalid_argument("Desktop surface size must be positive logical pixels up to 16384");
    if(!within(placement.margin_x, 0, max_margin) || !within(placement.margin_y, 0, max_margin))
        throw std::invalid_argument("Desktop surface margins must be non-negative logical pixels");
}

bool usable(const DesktopMonitor& monitor) {
    return monitor.width > 0 && monitor.height > 0 && monitor.width <= max_coordinate && monitor.height <= max_coordinate
        && std::abs(monitor.x) <= max_coordinate && std::abs(monitor.y) <= max_coordinate
        && within(monitor.dpi_scale, min_dpi_scale, max_dpi_scale);
}

int physical(float logical, float scale) { return static_cast<int>(std::lround(logical * scale)); }

struct Size { int width, height; };

// The surface on this monitor: its DPI decides the pixels, its work area caps them.
Size size_on(const DesktopPlacement& placement, const DesktopMonitor& monitor) {
    return {std::clamp(physical(placement.width, monitor.dpi_scale), 1, monitor.width),
            std::clamp(physical(placement.height, monitor.dpi_scale), 1, monitor.height)};
}

DesktopFrame clamp_into(const DesktopMonitor& monitor, int x, int y, Size size) {
    return {std::clamp(x, monitor.x, monitor.x + monitor.width - size.width),
            std::clamp(y, monitor.y, monitor.y + monitor.height - size.height),
            size.width, size.height, monitor.dpi_scale, false};
}

bool on_left(DesktopCorner corner) { return corner == DesktopCorner::top_left || corner == DesktopCorner::bottom_left; }
bool on_top(DesktopCorner corner) { return corner == DesktopCorner::top_left || corner == DesktopCorner::top_right; }

DesktopFrame in_corner(const DesktopPlacement& placement, const DesktopMonitor& monitor) {
    const auto size = size_on(placement, monitor);
    const int margin_x = physical(placement.margin_x, monitor.dpi_scale);
    const int margin_y = physical(placement.margin_y, monitor.dpi_scale);
    const int x = on_left(placement.corner) ? monitor.x + margin_x : monitor.x + monitor.width - margin_x - size.width;
    const int y = on_top(placement.corner) ? monitor.y + margin_y : monitor.y + monitor.height - margin_y - size.height;
    // A margin wider than the work area still has to leave the surface on screen.
    return clamp_into(monitor, x, y, size);
}

// Share of an absolute surface, sized for this monitor, that lies on its work area.
double share_on(const DesktopPlacement& placement, const DesktopMonitor& monitor) {
    const auto size = size_on(placement, monitor);
    const double left = std::max<double>(placement.x, monitor.x);
    const double top = std::max<double>(placement.y, monitor.y);
    const double right = std::min<double>(static_cast<double>(placement.x) + size.width,
                                          static_cast<double>(monitor.x) + monitor.width);
    const double bottom = std::min<double>(static_cast<double>(placement.y) + size.height,
                                           static_cast<double>(monitor.y) + monitor.height);
    if(right <= left || bottom <= top) return 0;
    return (right - left) * (bottom - top) / (static_cast<double>(size.width) * size.height);
}

struct Settled { DesktopFrame frame; const DesktopMonitor* monitor; };

Settled settle(const DesktopPlacement& placement, const std::vector<DesktopMonitor>& monitors) {
    validate(placement);
    const DesktopMonitor* primary = nullptr;
    const DesktopMonitor* holder = nullptr;
    double best = 0;
    for(const auto& monitor : monitors) {
        if(!usable(monitor)) continue;
        if(!primary || (monitor.primary && !primary->primary)) primary = &monitor;
        if(!placement.absolute) continue;
        const double share = share_on(placement, monitor);
        // Equal shares prefer the primary monitor, then the order the host listed them in.
        if(share > best || (share > 0 && share == best && monitor.primary && !holder->primary)) {
            best = share; holder = &monitor;
        }
    }
    if(!primary) throw std::invalid_argument("No usable monitor to place a desktop surface on");
    if(!placement.absolute) return {in_corner(placement, *primary), primary};
    if(!holder) {
        // The spot is on no monitor (it was unplugged or the layout changed): return to the
        // same corner of the primary rather than presenting where nobody can see it.
        auto frame = in_corner(placement, *primary);
        frame.recovered = true;
        return {frame, primary};
    }
    return {clamp_into(*holder, placement.x, placement.y, size_on(placement, *holder)), holder};
}
}

DesktopCorner desktop_corner_from_name(std::string_view name) {
    for(const auto& [text, value] : corner_names) if(name == text) return value;
    throw std::invalid_argument("Corner must be top-left, top-right, bottom-left or bottom-right");
}

const char* desktop_corner_name(DesktopCorner corner) noexcept {
    for(const auto& [text, value] : corner_names) if(corner == value) return text.data();
    return "bottom-right";
}

DesktopFrame place_on_desktop(const DesktopPlacement& placement, const std::vector<DesktopMonitor>& monitors) {
    return settle(placement, monitors).frame;
}

DesktopPlacement placement_at(const DesktopPlacement& current, int x, int y,
                              const std::vector<DesktopMonitor>& monitors) {
    auto moved = current;
    moved.absolute = true;
    moved.x = x; moved.y = y;
    const auto [frame, monitor] = settle(moved, monitors);
    moved.x = frame.x; moved.y = frame.y;
    // A recovered surface keeps the corner it returned to.
    if(frame.recovered) return moved;
    // Compare doubled coordinates so the centre needs no rounding.
    const bool left = 2LL * frame.x + frame.width < 2LL * monitor->x + monitor->width;
    const bool top = 2LL * frame.y + frame.height < 2LL * monitor->y + monitor->height;
    moved.corner = left ? (top ? DesktopCorner::top_left : DesktopCorner::bottom_left)
                        : (top ? DesktopCorner::top_right : DesktopCorner::bottom_right);
    const int gap_x = left ? frame.x - monitor->x : monitor->x + monitor->width - frame.x - frame.width;
    const int gap_y = top ? frame.y - monitor->y : monitor->y + monitor->height - frame.y - frame.height;
    // Capped so the reported placement is always one that place_on_desktop accepts back.
    moved.margin_x = std::min(static_cast<float>(gap_x) / monitor->dpi_scale, max_margin);
    moved.margin_y = std::min(static_cast<float>(gap_y) / monitor->dpi_scale, max_margin);
    return moved;
}
}
