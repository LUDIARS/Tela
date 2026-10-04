// @spec SPEC-TL-DESKTOP-OVERLAY
#include <tela/macos_desktop_overlay.hpp>
#include "bitmap_view.hpp"
#include "desktop_panel.hpp"
#include "desktop_screens.hpp"
#include "input_panel.hpp"
#include "region_panels.hpp"
#include "window_target.hpp"
#include <cmath>
#include <optional>
#include <stdexcept>
#include <utility>

namespace tela {
namespace {
constexpr char host_id[] = "desktop";
constexpr char view_id[] = "surface";

// A drag in progress: where the pointer went down (AppKit global points) and how far the panels
// have been moved, in whole points so the reported spot is exactly where they stand.
struct Drag { NSPoint start{}; CGFloat dx{}, dy{}; };
// A top-left in global points, measured down from the primary display's top edge.
struct Spot { int x{}, y{}; };

// The primary button as AppKit reports it, read without an event so a release the grip never
// received is still noticed.
bool primary_button_down() { return (NSEvent.pressedMouseButtons & 1) != 0; }

int whole(CGFloat value) { return static_cast<int>(std::lround(value)); }
}

struct MacOSDesktopOverlay::Impl {
    Runtime& runtime;
    PictorSurface& renderer;
    DesktopPlacement placement;
    std::string grip;
    std::function<void(const DesktopPlacement&)> moved;
    const bool exclude_from_capture;
    macos::PointerSession pointer;
    NSPanel* visual{};
    // Declared after the pointer session its panels point at, so they are closed first.
    macos::RegionPanels regions;
    std::optional<Drag> drag;
    // A finished drag's top-left, applied by the next synchronize().
    std::optional<Spot> dropped;
    bool visible{};

    Impl(Runtime& r, PictorSurface& p, DesktopOverlayOptions options)
        : runtime(r), renderer(p), placement(options.placement), grip(std::move(options.grip)),
          moved(std::move(options.moved)), exclude_from_capture(options.exclude_from_capture), pointer{r},
          regions([capture = options.exclude_from_capture](NSPanel* panel) { macos::float_on_desktop(panel, capture); }) {
        macos::require_main_thread();
        // A size or margin no layout can satisfy fails here, before any panel exists.
        const auto screens = macos::desktop_screens();
        place_on_point_desktop(placement, screens.displays, screens.primary_top);
        // The surface belongs to no application in front, and its grip is not a runtime input.
        pointer.front_only = false;
        pointer.intercept = [this](NSEvent* event, PointerPhase phase) { return intercept(event, phase); };
        visual = macos::make_panel(true);
        macos::float_on_desktop(visual, exclude_from_capture);
    }
    ~Impl() {
        // Public contract requires destruction on the AppKit main thread.
        hide();
        regions.clear();
        [visual close];
        // The surface is gone, so presentation and input ownership end as for a lost host.
        runtime.disconnect();
    }
    void hide() {
        runtime.cancel();
        // Closing the grip's panel ends a drag; its drop is kept for the next synchronize().
        finish_drag();
        regions.clear();
        [visual orderOut:nil];
        visible = false;
    }

    // Native events reach this before the runtime. The grip moves the surface; between a drop and
    // the next synchronize() the panels sit at the new spot but the viewport does not, so other
    // input would land on the wrong element and is dropped.
    bool intercept(NSEvent* event, PointerPhase phase) {
        if (phase == PointerPhase::down && !grip.empty() && regions.region(event.window) == grip) {
            // Moving the surface is not a click on it: whatever gesture the runtime held ends here.
            runtime.cancel();
            drag = Drag{NSEvent.mouseLocation};
            return true;
        }
        if (drag) {
            // AppKit sends drag and up to the panel that took down, wherever the pointer is.
            if (phase == PointerPhase::move) drag_to();
            else if (phase == PointerPhase::up) { drag_to(); finish_drag(); }
            return true;
        }
        return dropped.has_value();
    }
    void drag_to() {
        const NSPoint at = NSEvent.mouseLocation;
        const CGFloat dx = std::round(at.x - drag->start.x), dy = std::round(at.y - drag->start.y);
        if (dx == drag->dx && dy == drag->dy) return;
        const CGFloat step_x = dx - drag->dx, step_y = dy - drag->dy;
        regions.offset(step_x, step_y);
        [visual setFrameOrigin:NSMakePoint(visual.frame.origin.x + step_x, visual.frame.origin.y + step_y)];
        drag->dx = dx; drag->dy = dy;
    }
    void finish_drag() {
        if (!drag) return;
        const auto finished = *drag;
        drag.reset();
        if (!finished.dx && !finished.dy) return;
        // A drop not yet applied is where the panels started this time.
        const auto& frame = pointer.target.frame;
        const Spot base = dropped.value_or(Spot{whole(frame.origin.x), whole(pointer.target.primary_top - NSMaxY(frame))});
        // AppKit's y grows upwards; the placement's grows downwards.
        dropped = Spot{base.x + whole(finished.dx), base.y - whole(finished.dy)};
    }

    // Reports the frame as the viewport; unchanged geometry keeps the current revision and panels.
    void observe(const PointFrame& frame, double primary_top) {
        const NSRect bounds = NSMakeRect(frame.appkit_x, frame.appkit_y, frame.points.width, frame.points.height);
        auto next = runtime.viewport();
        const bool same = next.host_id == host_id && next.view_id == view_id
            && next.desktop_x == frame.pixel_x && next.desktop_y == frame.pixel_y
            && next.width == frame.pixel_width && next.height == frame.pixel_height
            && next.dpi_scale == frame.points.dpi_scale && next.visible && next.focused
            && NSEqualRects(pointer.target.frame, bounds) && pointer.target.primary_top == primary_top;
        if (!same) {
            // The input panels stand where the old frame was: end their gesture and drop them.
            runtime.cancel();
            regions.clear();
            ++next.revision;
            next.host_id = host_id; next.view_id = view_id;
            next.desktop_x = frame.pixel_x; next.desktop_y = frame.pixel_y;
            next.width = frame.pixel_width; next.height = frame.pixel_height;
            next.dpi_scale = frame.points.dpi_scale;
            // A surface that is never activated has no host focus to lose, so it takes input while shown.
            next.visible = true; next.focused = true;
            runtime.viewport(next);
            // Presenting again moves the visual panel too, even when only the AppKit origin moved.
            runtime.invalidate();
        }
        pointer.target.frame = bounds;
        pointer.target.primary_top = primary_top;
        pointer.target.viewport = runtime.viewport();
        pointer.target.exists = true;
    }
    void synchronize() {
        macos::require_main_thread();
        try {
            // Report a stored input-callback failure exactly once, as MacOSOverlay does.
            if (pointer.error) {
                const auto stored = std::exchange(pointer.error, {});
                std::rethrow_exception(stored);
            }
            // A release the grip never saw (the panel lost the mouse) still ends the drag.
            if (drag && !primary_button_down()) finish_drag();
            // The panels follow the pointer during a drag; the drop presents at the new spot.
            if (drag) return;
            const auto screens = macos::desktop_screens();
            if (screens.displays.empty()) { if (visible) hide(); return; }
            bool settled = false;
            if (dropped) {
                placement = point_placement_at(placement, dropped->x, dropped->y, screens.displays, screens.primary_top);
                dropped.reset();
                settled = true;
            }
            observe(place_on_point_desktop(placement, screens.displays, screens.primary_top), screens.primary_top);
            if (!visible || runtime.needs_frame()) {
                auto surface = renderer.render(runtime);
                regions.rebuild(runtime, pointer);
                regions.present(surface, pointer.target);
                macos::present_bitmap(visual, surface, pointer.target.frame, runtime.viewport().dpi_scale);
                runtime.frame_presented();
                visible = true;
            }
            if (settled && moved) moved(placement);
        } catch (...) { hide(); throw; }
    }
    void place(const DesktopPlacement& next) {
        const auto screens = macos::desktop_screens();
        place_on_point_desktop(next, screens.displays, screens.primary_top);
        placement = next;
        // An explicit placement replaces a drop that has not been applied yet.
        dropped.reset();
    }
};

MacOSDesktopOverlay::MacOSDesktopOverlay(Runtime& runtime, PictorSurface& renderer, DesktopOverlayOptions options)
    : impl_(std::make_unique<Impl>(runtime, renderer, std::move(options))) {}
MacOSDesktopOverlay::~MacOSDesktopOverlay() = default;
void MacOSDesktopOverlay::synchronize() { impl_->synchronize(); }
void MacOSDesktopOverlay::hide() { macos::require_main_thread(); impl_->hide(); }
void MacOSDesktopOverlay::place(const DesktopPlacement& placement) { macos::require_main_thread(); impl_->place(placement); }
const DesktopPlacement& MacOSDesktopOverlay::placement() const noexcept { return impl_->placement; }
std::string MacOSDesktopOverlay::hovered() const {
    if (!impl_->visible) return {};
    return impl_->regions.under(NSEvent.mouseLocation);
}
std::uintptr_t MacOSDesktopOverlay::native_window() const noexcept {
    return static_cast<std::uintptr_t>(impl_->visual.windowNumber);
}
}
