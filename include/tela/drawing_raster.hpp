#pragma once
#include <tela/drawing.hpp>
#include <tela/pixel_surface.hpp>
namespace tela {
// @spec Overlay drawing
// CPU adapter shared by platform hosts; origin is logical, clip is physical.
void paint_drawing(PixelSurface&,const Drawing&,Point origin,float dpi_scale,Rect clip);
}
