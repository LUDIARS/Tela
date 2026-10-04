// @spec SPEC-TL-MACOS
#pragma once
#include "input_panel.hpp"
#include <tela/pixel_surface.hpp>
#include <functional>
#include <string>
#include <vector>

namespace tela::macos {
// The input panels of one Tela surface: one nonactivating panel per fragment of each effective
// exclusive region, carrying that fragment's final pixels. MacOSOverlay and MacOSDesktopOverlay
// both take input through this, so there is one way to cut the exclusive regions out on macOS.
class RegionPanels {
public:
    // Applied to every panel right after it is made, for the window level, Spaces behaviour and
    // capture exclusion the owning surface needs. Empty keeps make_input_panel's defaults.
    using Configure = std::function<void(NSPanel*)>;
    explicit RegionPanels(Configure configure = {});
    ~RegionPanels();
    RegionPanels(const RegionPanels&) = delete;
    RegionPanels& operator=(const RegionPanels&) = delete;

    // Replaces the panels with the runtime's current exclusive regions. While the runtime holds a
    // captured gesture nothing changes: AppKit keeps sending drag/up to the panel that took down.
    void rebuild(Runtime&, PointerSession&);
    // Moves each panel's pixels out of `visual` into the panel and shows it, so no pixel is
    // composited twice.
    void present(PixelSurface& visual, const WindowTarget&);
    // Moves every panel by (dx, dy) points without presenting again.
    void offset(CGFloat dx, CGFloat dy);
    // Closes every panel. Safe to call more than once.
    void clear();
    // The exclusive region `window` shows; empty for any window it does not own.
    std::string region(NSWindow* window) const;
    // The exclusive region of a shown panel under `point` (AppKit global points); empty elsewhere.
    std::string under(NSPoint point) const;
private:
    Configure configure_;
    NSMutableArray<NSPanel*>* panels_;
    std::vector<std::string> ids_; // parallel to panels_
};
}
