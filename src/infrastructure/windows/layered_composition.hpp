#pragma once
// @implements SPEC-TL-OVERLAY
// @spec Overlay lifecycle
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <tela/pixel_surface.hpp>
#include <tela/runtime.hpp>
#include <functional>
#include <string>
#include <unordered_map>

namespace tela::windows {
// The translucent windows a Tela surface is shown with: one passthrough visual window, plus one
// input window per exclusive region, clipped to that region's fragments. Every layered Tela
// surface presents through this, so there is one way to show per-pixel alpha and one way to
// take input only where the declaration asked for it.
class LayeredComposition {
public:
    // Makes one window of the owner's class, passthrough or input-taking. The composition
    // destroys every window it made.
    using Create = std::function<HWND(bool passthrough)>;
    explicit LayeredComposition(Create create);
    ~LayeredComposition();
    LayeredComposition(const LayeredComposition&) = delete;
    LayeredComposition& operator=(const LayeredComposition&) = delete;

    // Creates the visual window. Separate from construction so the owner can register its
    // window class first.
    void open();
    // Shows the surface at the runtime's viewport. Region windows carry the exclusive pixels and
    // the visual carries the rest, so Windows never blends a pixel twice. A region that is no
    // longer declared is destroyed, cancelling the gesture it held.
    void present(Runtime&, const PixelSurface&);
    // Hides every window and releases capture held by any of them.
    void hide();
    // Moves every window by (dx, dy) without presenting again.
    void offset(int dx, int dy);
    // Destroys every window. Safe to call more than once.
    void destroy();
    HWND visual() const noexcept;
    // The exclusive region `window` shows; empty for the visual or a window it does not own.
    std::string region(HWND window) const;
private:
    Create create_;
    HWND visual_{};
    std::unordered_map<std::string, HWND> regions_;
};
}
