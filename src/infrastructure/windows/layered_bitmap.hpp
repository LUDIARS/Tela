#pragma once
// @spec SPEC-TL-OVERLAY
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <tela/pictor_surface.hpp>

namespace tela::windows {
void present(HWND window, const PixelSurface&, int desktop_x, int desktop_y,
             int source_x = 0, int source_y = 0, int width = 0, int height = 0,
             bool hit_region = false);
}
