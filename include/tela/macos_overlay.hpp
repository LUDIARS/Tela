// @spec SPEC-TL-MACOS
#pragma once
#include <tela/pictor_surface.hpp>
#include <memory>

namespace tela {
// AppKit main-thread adapter. The caller owns NSApplication and its event loop.
// target_window is a CGWindowID, not an NSWindow pointer. Runtime and renderer
// must outlive this adapter. One adapter owns its runtime's viewport.
class MacOSOverlay {
public:
    MacOSOverlay(Runtime&, PictorSurface&, std::uint32_t target_window);
    ~MacOSOverlay();
    MacOSOverlay(const MacOSOverlay&) = delete;
    MacOSOverlay& operator=(const MacOSOverlay&) = delete;
    void synchronize(); // call on host events/deadlines, no internal timer
    void hide(); // suspend until the next synchronize; cancels native gestures
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
