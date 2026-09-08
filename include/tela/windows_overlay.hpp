#pragma once
#include <tela/pictor_surface.hpp>
#include <memory>

namespace tela {
// Explicit HWND boundary confined to this optional Windows adapter.
class WindowsOverlay {
public:
    WindowsOverlay(Runtime&, PictorSurface&, std::uintptr_t target_window);
    ~WindowsOverlay();
    WindowsOverlay(const WindowsOverlay&) = delete;
    WindowsOverlay& operator=(const WindowsOverlay&) = delete;
    void synchronize(); // geometry/visibility, then present only when dirty
    void hide();
    std::uintptr_t native_window() const noexcept;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
