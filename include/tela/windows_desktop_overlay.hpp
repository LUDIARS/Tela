// @implements SPEC-TL-DESKTOP-OVERLAY
// @spec Desktop overlay
#pragma once
#include <tela/desktop_overlay_options.hpp>
#include <tela/pictor_surface.hpp>
#include <cstdint>
#include <memory>
#include <string>

namespace tela {
// A surface with no target window, floating on the desktop: topmost, never activated, absent
// from the taskbar and Alt+Tab, with per-pixel alpha. It presents the renderer's premultiplied
// bitmap as is, so any translucency comes from the declaration's own colours. Input passes
// through to the windows below except over the declaration's exclusive regions. Monitor
// geometry and DPI are read in Per-Monitor V2 whatever the process default is.
class WindowsDesktopOverlay {
public:
    // Throws std::invalid_argument when no monitor layout can hold the placement (see
    // place_on_desktop) and std::runtime_error when the windows cannot be created. With
    // exclude_from_capture, a window Windows cannot exclude (before Windows 10 2004) throws
    // std::runtime_error from here or from the synchronize() that creates it; it never falls
    // back to WDA_MONITOR.
    WindowsDesktopOverlay(Runtime&, PictorSurface&, DesktopOverlayOptions);
    // Releases every window, capture and surface; the runtime loses the surface as a lost host.
    ~WindowsDesktopOverlay();
    WindowsDesktopOverlay(const WindowsDesktopOverlay&) = delete;
    WindowsDesktopOverlay& operator=(const WindowsDesktopOverlay&) = delete;
    // Places the surface for the current monitors and DPI, reports a finished drag, then presents
    // only when the declaration changed or the surface was hidden. Nothing is presented mid-drag.
    // Call it from the message loop regularly: it is how monitor and DPI changes are noticed.
    void synchronize();
    // Hides the surface until the next synchronize().
    void hide();
    // Replaces the placement (size included); the next synchronize() applies it.
    void place(const DesktopPlacement&);
    const DesktopPlacement& placement() const noexcept;
    // ID of the exclusive region under the pointer, empty when the pointer is elsewhere.
    std::string hovered() const;
    std::uintptr_t native_window() const noexcept;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
