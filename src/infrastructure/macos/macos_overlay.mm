// @spec SPEC-TL-MACOS
#include <tela/macos_overlay.hpp>
#include "window_target.hpp"
#include "bitmap_view.hpp"
#include "input_panel.hpp"
#include "region_panels.hpp"
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
    macos::RegionPanels regions;
    bool visible{}, lost{};
    Impl(Runtime& r, PictorSurface& p, std::uint32_t w)
        : runtime(r), renderer(p), window(w), pointer{r} {
        macos::require_main_thread();
        if (!w) throw std::invalid_argument("Tela requires a nonzero CGWindowID");
        pointer.target = macos::observe_window(w, 0);
        if (!pointer.target.exists) throw std::invalid_argument("Tela target window does not exist");
        process = pointer.target.process;
        visual = macos::make_panel(true);
    }
    ~Impl() {
        // Public contract requires destruction on the AppKit main thread.
        hide(); [visual close];
    }
    void hide() {
        runtime.cancel(); regions.clear(); [visual orderOut:nil]; visible = false;
    }
    void update_target() {
        auto next = macos::observe_window(window, process);
        if (!next.exists) lost = true; // never reattach a reused window number
        auto& v = next.viewport;
        const auto& old = runtime.viewport();
        const bool changed = !macos::same_geometry(v, old) || v.visible != old.visible ||
            v.focused != old.focused || v.host_id != old.host_id || v.view_id != old.view_id;
        if (changed) { runtime.cancel(); regions.clear(); }
        v.revision = old.revision + (changed ? 1 : 0);
        runtime.viewport(v);
        pointer.target = std::move(next);
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
            regions.rebuild(runtime, pointer);
            regions.present(surface, pointer.target);
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
