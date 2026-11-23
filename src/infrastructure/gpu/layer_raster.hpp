#pragma once
// @implements SPEC-TL-GPU
// @spec GPU rendering
#include "../pictor/pictor_text.hpp"
#include <tela/gpu_frame.hpp>

namespace tela::gpu {
// Rasterizes a layer's items into `out` (cleared first, premultiplied BGRA of the frame's size)
// with the same text and shape rasterizers the CPU surface uses.
void raster_layer(PixelSurface& out, const GpuLayer&, pictor_adapter::PictorText&);
}
