#pragma once
// @implements SPEC-TL-CONTROLS
// @spec Controls
#include "pictor_text.hpp"
#include <tela/control_visual.hpp>
#include <tela/runtime.hpp>

namespace tela::pictor_adapter {
// Physical-pixel fill of `rect` limited to `clip`.
void fill(PixelSurface&, Rect rect, Rect clip, Color);
// Rasterizes control_visual's items; `clip` is physical, the items are logical.
void paint_control(PixelSurface&, PictorText&, const Runtime&, const PlacedElement&, Rect clip);
}
