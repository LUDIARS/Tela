#pragma once
// @spec Overlay drawing
#include <algorithm>
namespace tela::raster {
inline void blend(unsigned char* dst,unsigned char b,unsigned char g,unsigned char r,unsigned char a) {
    const unsigned inverse=255-a;
    dst[0]=static_cast<unsigned char>(std::min(255u,b+(dst[0]*inverse+127)/255));
    dst[1]=static_cast<unsigned char>(std::min(255u,g+(dst[1]*inverse+127)/255));
    dst[2]=static_cast<unsigned char>(std::min(255u,r+(dst[2]*inverse+127)/255));
    dst[3]=static_cast<unsigned char>(std::min(255u,a+(dst[3]*inverse+127)/255));
}
}
