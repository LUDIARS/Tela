#pragma once
#include <tela/runtime.hpp>
#include <tela/pixel_surface.hpp>
#include <memory>

namespace tela {
// Explicit CPU composition backend using Pictor's TrueType rasterizer.
class PictorSurface {
public:
    explicit PictorSurface(const std::string& true_type_font);
    ~PictorSurface();
    PictorSurface(const PictorSurface&) = delete;
    PictorSurface& operator=(const PictorSurface&) = delete;
    PixelSurface render(const Runtime&);
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
