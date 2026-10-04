// @implements SPEC-TL-DESKTOP-OVERLAY
// @spec Desktop overlay
#pragma once
#include <tela/desktop_placement.hpp>
#include <functional>
#include <string>

namespace tela {
// What a desktop surface is asked for, shared by every OS adapter (WindowsDesktopOverlay,
// MacOSDesktopOverlay) so a caller describes the surface once.
struct DesktopOverlayOptions {
    DesktopPlacement placement;
    // Element ID whose exclusive region drags the surface. Its pointer input moves the surface
    // and never reaches the runtime, so it should not be a button. Empty: the surface stays put.
    std::string grip;
    // Where the surface settled after a drag. Storing it is the caller's job; passing it back as
    // the placement restores the spot. Called from synchronize(), never from a native event,
    // and it must not destroy the surface.
    std::function<void(const DesktopPlacement&)> moved;
    // Keeps every window of the surface out of screen capture and recording (a presenter's own
    // overlay stays off the stream). When the OS cannot exclude a window the adapter throws
    // std::runtime_error instead of showing it in the capture.
    bool exclude_from_capture{};
};
}
