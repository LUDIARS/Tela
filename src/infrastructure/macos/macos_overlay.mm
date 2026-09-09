// @spec SPEC-TL-MACOS
#include <tela/macos_overlay.hpp>
#include <tela/input_regions.hpp>
#include "window_target.hpp"
#include "bitmap_view.hpp"
#include "input_panel.hpp"
#include "region_surface.hpp"
#include <stdexcept>
#include <utility>

namespace tela {
struct MacOSOverlay::Impl {
    Runtime& runtime;
    PictorSurface& renderer;
    const std::uint32_t window;
    int process{};
    macos::PointerSession pointer;
    NSPanel* visual;
    NSMutableArray<NSPanel*>* regions;
    bool visible{}, lost{};
    Impl(Runtime& r, PictorSurface& p, std::uint32_t w)
        : runtime(r), renderer(p), window(w), pointer{r} {
        macos::require_main_thread();
        if (!w) throw std::invalid_argument("Tela requires a nonzero CGWindowID");
        pointer.target = macos::observe_window(w, 0);
        if (!pointer.target.exists) throw std::invalid_argument("Tela target window does not exist");
        process = pointer.target.process;
        visual = macos::make_panel(true);
        regions = [NSMutableArray array];
    }
    ~Impl() {
        // Public contract requires destruction on the AppKit main thread.
        hide(); [visual close];
    }
    void clear_regions() {
        for (NSPanel* panel in regions) macos::close_input_panel(panel);
        [regions removeAllObjects];
    }
    void hide() {
        runtime.cancel(); clear_regions(); [visual orderOut:nil]; visible = false;
    }
    void update_target() {
        auto next = macos::observe_window(window, process);
        if (!next.exists) lost = true; // never reattach a reused window number
        auto& v = next.viewport;
        const auto& old = runtime.viewport();
        const bool changed = !macos::same_geometry(v, old) || v.visible != old.visible ||
            v.focused != old.focused || v.host_id != old.host_id || v.view_id != old.view_id;
        if (changed) { runtime.cancel(); clear_regions(); }
        v.revision = old.revision + (changed ? 1 : 0);
        runtime.viewport(v);
        pointer.target = std::move(next);
    }
    void rebuild_regions() {
        if (runtime.captured()) return; // preserve the native down recipient
        NSMutableArray<NSPanel*>* next = [NSMutableArray array];
        try {
            for (const auto& region : exclusive_regions(runtime.elements())) {
                for (const auto fragment : region.fragments) {
                    if (fragment.width <= 0 || fragment.height <= 0) continue;
                    NSRect bounds = macos::fragment_frame(fragment, pointer.target);
                    if (NSIsEmptyRect(bounds)) continue;
                    [next addObject:macos::make_input_panel(pointer, bounds)];
                }
            }
        } catch (...) { for (NSPanel* panel in next) macos::close_input_panel(panel); throw; }
        clear_regions(); regions = next;
    }
    void synchronize() {
        macos::require_main_thread();
        if (lost) { runtime.disconnect(); hide(); return; }
        try {
            // Report a stored input-callback failure exactly once: clearing it
            // before the throw lets a later synchronize() resume, as hide() and
            // the public contract promise, instead of rethrowing it forever.
            if (pointer.error) {
                const auto stored = std::exchange(pointer.error, {});
                std::rethrow_exception(stored);
            }
            update_target();
            if (!runtime.viewport().visible || lost) { hide(); return; }
            if (!visible) runtime.invalidate();
            if (!runtime.needs_frame()) return;
            auto surface = renderer.render(runtime);
            rebuild_regions();
            macos::present_regions(regions, surface, pointer.target);
            macos::present_bitmap(visual, surface, pointer.target.frame, runtime.viewport().dpi_scale);
            runtime.frame_presented(); visible = true;
        } catch (...) { hide(); throw; }
    }
};
MacOSOverlay::MacOSOverlay(Runtime& r, PictorSurface& p, std::uint32_t window)
    : impl_(std::make_unique<Impl>(r,p,window)) {}
MacOSOverlay::~MacOSOverlay() = default;
void MacOSOverlay::synchronize() { impl_->synchronize(); }
void MacOSOverlay::hide() { macos::require_main_thread(); impl_->hide(); }
}
