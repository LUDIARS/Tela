// @implements SPEC-TL-VIEW-HOST
// @spec View host
#pragma once
#include <tela/pictor_surface.hpp>
#include <cstdint>
#include <memory>
#include <string>

namespace tela {
// A normal top-level window that draws the declaration itself instead of overlaying a host.
// It owns its whole surface and all of its input, so passthrough and exclusive regions do
// not apply here; closing it releases the declaration's input the same way a lost host does.
class WindowsView {
public:
    WindowsView(Runtime&, PictorSurface&, const std::string& title, int width, int height, bool fullscreen);
    ~WindowsView();
    WindowsView(const WindowsView&) = delete;
    WindowsView& operator=(const WindowsView&) = delete;
    // Processes pending messages. False once the window has been closed.
    bool pump();
    // Reports the client area as the viewport, then presents only when the declaration changed.
    void synchronize();
    void fullscreen(bool);
    bool fullscreen() const noexcept;
    std::uintptr_t native_window() const noexcept;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
