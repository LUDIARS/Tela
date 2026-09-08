// @spec SPEC-TL-OVERLAY
#include "layered_bitmap.hpp"
#include <cstring>
#include <stdexcept>

namespace tela::windows {
namespace {
struct Bitmap {
    HDC dc{CreateCompatibleDC(nullptr)};
    HBITMAP bitmap{};
    HGDIOBJ previous{};
    ~Bitmap() {
        if (previous) SelectObject(dc,previous);
        if (bitmap) DeleteObject(bitmap);
        if (dc) DeleteDC(dc);
    }
};
}
void present(HWND window, const PixelSurface& surface, int x, int y,
             int sx, int sy, int width, int height, bool hit_region) {
    if (!width) width = surface.width;
    if (!height) height = surface.height;
    if (width<=0 || height<=0) return;
    Bitmap owner;
    BITMAPINFO info{};
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = width; info.bmiHeader.biHeight = -height;
    info.bmiHeader.biPlanes = 1; info.bmiHeader.biBitCount = 32; info.bmiHeader.biCompression = BI_RGB;
    void* data{};
    owner.bitmap = CreateDIBSection(owner.dc,&info,DIB_RGB_COLORS,&data,nullptr,0);
    if (!owner.dc || !owner.bitmap) throw std::runtime_error("Cannot allocate layered window bitmap");
    owner.previous = SelectObject(owner.dc,owner.bitmap);
    auto* dst = static_cast<unsigned char*>(data);
    for (int row=0;row<height;++row) {
        std::memcpy(dst+static_cast<size_t>(row)*width*4,
            surface.pixels.data()+(static_cast<size_t>(sy+row)*surface.width+sx)*4,static_cast<size_t>(width)*4);
        // Transparent pixels within an exclusive region must still own input.
        if (hit_region) for (int col=0;col<width;++col)
            dst[(static_cast<size_t>(row)*width+col)*4+3] = std::max<unsigned char>(1,dst[(static_cast<size_t>(row)*width+col)*4+3]);
    }
    POINT destination{x,y}, source{}; SIZE size{width,height};
    BLENDFUNCTION alpha{AC_SRC_OVER,0,255,AC_SRC_ALPHA};
    if (!UpdateLayeredWindow(window,nullptr,&destination,&size,owner.dc,&source,0,&alpha,ULW_ALPHA))
        throw std::runtime_error("UpdateLayeredWindow failed: " + std::to_string(GetLastError()));
}
}
