// @spec SPEC-TL-MACOS
#pragma once
#include "window_target.hpp"
#include <tela/geometry.hpp>
#include <tela/pixel_surface.hpp>
namespace tela::macos {
NSRect fragment_frame(Rect, const WindowTarget&);
void present_regions(NSArray<NSPanel*>*, PixelSurface&, const WindowTarget&);
}
