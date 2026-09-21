// @spec SPEC-TL-OVERLAY
#include "layered_composition.hpp"
#include "layered_bitmap.hpp"
#include <tela/input_regions.hpp>
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace tela::windows {
LayeredComposition::LayeredComposition(Create create) : create_(std::move(create)) {}
LayeredComposition::~LayeredComposition() { destroy(); }

void LayeredComposition::open() {
    if(!visual_) visual_ = create_(true);
}

void LayeredComposition::present(Runtime& runtime, const PixelSurface& surface) {
    const auto& v=runtime.viewport();
    auto visualSurface=surface;
    std::unordered_map<std::string,bool> keep;
    for (const auto& e:exclusive_regions(runtime.elements())) {
        const auto rect=intersect(e.bounds,{0,0,v.width/v.dpi_scale,v.height/v.dpi_scale});
        const int x=std::clamp(static_cast<int>(std::floor(rect.x*v.dpi_scale)),0,v.width);
        const int y=std::clamp(static_cast<int>(std::floor(rect.y*v.dpi_scale)),0,v.height);
        const int width=std::min(v.width-x,static_cast<int>(std::ceil(rect.width*v.dpi_scale)));
        const int height=std::min(v.height-y,static_cast<int>(std::ceil(rect.height*v.dpi_scale)));
        if (width<=0 || height<=0) continue;
        auto it=regions_.find(e.id);
        if (it==regions_.end()) it=regions_.emplace(e.id,create_(false)).first;
        keep[e.id]=true;
        // The region window carries the final composited pixels for its
        // fragments; SetWindowRgn below clips it to exactly those fragments,
        // and the same rects are cleared from visualSurface so Windows never
        // blends a pixel twice.
        windows::present(it->second,surface,v.desktop_x+x,v.desktop_y+y,x,y,width,height,true);
        HRGN mask=CreateRectRgn(0,0,0,0);
        if(!mask)throw std::runtime_error("Cannot allocate hit region");
        for(auto fragment:e.fragments){
            const int left=std::clamp(static_cast<int>(std::ceil(fragment.x*v.dpi_scale)),x,x+width);
            const int top=std::clamp(static_cast<int>(std::ceil(fragment.y*v.dpi_scale)),y,y+height);
            const int right=std::clamp(static_cast<int>(std::ceil((fragment.x+fragment.width)*v.dpi_scale)),left,x+width);
            const int bottom=std::clamp(static_cast<int>(std::ceil((fragment.y+fragment.height)*v.dpi_scale)),top,y+height);
            HRGN part=CreateRectRgn(left-x,top-y,right-x,bottom-y);
            if(!part){DeleteObject(mask);throw std::runtime_error("Cannot allocate hit fragment");}
            const auto merged=CombineRgn(mask,mask,part,RGN_OR);DeleteObject(part);
            if(merged==ERROR){DeleteObject(mask);throw std::runtime_error("Cannot combine hit fragments");}
            for(int row=top;row<bottom;++row)
                std::fill_n(visualSurface.pixels.begin()+(static_cast<size_t>(row)*v.width+left)*4,static_cast<size_t>(right-left)*4,0);
        }
        // Windows owns mask after a successful SetWindowRgn.
        if(!SetWindowRgn(it->second,mask,FALSE)){DeleteObject(mask);throw std::runtime_error("Cannot apply hit mask");}
        SetWindowPos(it->second,HWND_TOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE|SWP_SHOWWINDOW);
    }
    for (auto it=regions_.begin();it!=regions_.end();) {
        if (!keep.contains(it->first)) {
            if (GetCapture()==it->second) { runtime.cancel(); ReleaseCapture(); }
            DestroyWindow(it->second); it=regions_.erase(it);
        } else ++it;
    }
    windows::present(visual_,visualSurface,v.desktop_x,v.desktop_y);
    SetWindowPos(visual_,HWND_TOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE|SWP_SHOWWINDOW);
}

void LayeredComposition::hide() {
    if (visual_ && GetCapture()==visual_) ReleaseCapture();
    for (auto [id,w] : regions_) { if (GetCapture()==w) ReleaseCapture(); ShowWindow(w,SW_HIDE); }
    if (visual_) ShowWindow(visual_,SW_HIDE);
}

void LayeredComposition::offset(int dx, int dy) {
    if(!dx && !dy) return;
    const auto move = [&](HWND window) {
        RECT bounds{};
        if(GetWindowRect(window, &bounds))
            SetWindowPos(window, nullptr, bounds.left + dx, bounds.top + dy, 0, 0,
                         SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    };
    for(const auto& [id, window] : regions_) move(window);
    if(visual_) move(visual_);
}

void LayeredComposition::destroy() {
    // Erase each entry before destroying its window: DestroyWindow dispatches messages to the
    // owner, which may ask which region a window shows while the map is being emptied.
    while(!regions_.empty()) {
        const auto window = regions_.begin()->second;
        regions_.erase(regions_.begin());
        DestroyWindow(window);
    }
    if(const auto window = std::exchange(visual_, nullptr)) DestroyWindow(window);
}

HWND LayeredComposition::visual() const noexcept { return visual_; }

std::string LayeredComposition::region(HWND window) const {
    for(const auto& [id, owned] : regions_) if(owned == window) return id;
    return {};
}
}
