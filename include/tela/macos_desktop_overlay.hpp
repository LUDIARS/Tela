// @implements SPEC-TL-DESKTOP-OVERLAY
// @spec Desktop overlay
#pragma once
#include <tela/desktop_overlay_options.hpp>
#include <tela/pictor_surface.hpp>
#include <cstdint>
#include <memory>
#include <string>

namespace tela {
// The macOS desktop surface: no target window, floating above other apps on every Space as
// nonactivating panels that never take focus and stay out of Mission Control's window spread and
// the window cycle. It presents the renderer's premultiplied bitmap as is, so any translucency
// comes from the declaration's own colours, and lets input through except over the declaration's
// exclusive regions.
//
// Placement is in points: sizes and margins are logical points, a corner is a corner of the
// primary display's visibleFrame (below the menu bar, above the Dock), and an absolute spot is the
// top-left in global points measured down from the primary display's top edge. The surface is
// drawn at the backingScaleFactor of the display holding it.
//
// AppKit main-thread adapter like MacOSOverlay: the caller owns NSApplication and its event loop,
// and creates, synchronizes, hides and destroys it on the main thread. Runtime and renderer must
// outlive it. Whether the application itself shows in the Dock and Cmd+Tab is the caller's
// activation policy (NSApplicationActivationPolicyAccessory keeps it out).
class MacOSDesktopOverlay {
public:
    // Throws std::invalid_argument when no display layout can hold the placement (see
    // place_on_point_desktop), std::logic_error off the main thread and std::runtime_error when
    // a panel cannot be created.
    MacOSDesktopOverlay(Runtime&, PictorSurface&, DesktopOverlayOptions);
    // Closes every panel; the runtime loses the surface as a lost host.
    ~MacOSDesktopOverlay();
    MacOSDesktopOverlay(const MacOSDesktopOverlay&) = delete;
    MacOSDesktopOverlay& operator=(const MacOSDesktopOverlay&) = delete;
    // Places the surface for the current displays and backing scales, reports a finished drag,
    // then presents only when the declaration changed or the surface was hidden. Nothing is
    // presented mid-drag. Call it from the event loop regularly: it is how display changes are
    // noticed. An input-callback failure is rethrown once from here and hides the surface.
    void synchronize();
    // Hides the surface until the next synchronize().
    void hide();
    // Replaces the placement (size included); the next synchronize() applies it.
    void place(const DesktopPlacement&);
    const DesktopPlacement& placement() const noexcept;
    // ID of the exclusive region under the pointer, empty when the pointer is elsewhere.
    std::string hovered() const;
    // CGWindowID of the visual panel, for a capture that wants to name the surface explicitly.
    std::uintptr_t native_window() const noexcept;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
