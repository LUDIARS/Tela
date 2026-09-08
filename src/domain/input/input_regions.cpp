// @spec SPEC-TL-INPUT
#include <tela/input_regions.hpp>
#include <stdexcept>

namespace tela {
namespace {
void subtract(Rect a,Rect b,std::vector<Rect>& out){
    auto overlap=intersect(a,b);if(overlap.width<=0||overlap.height<=0){out.push_back(a);return;}
    auto append=[&](Rect r){if(r.width>0&&r.height>0)out.push_back(r);};
    append({a.x,a.y,a.width,overlap.y-a.y});
    append({a.x,overlap.y+overlap.height,a.width,a.y+a.height-overlap.y-overlap.height});
    append({a.x,overlap.y,overlap.x-a.x,overlap.height});
    append({overlap.x+overlap.width,overlap.y,a.x+a.width-overlap.x-overlap.width,overlap.height});
}
}
std::vector<InputRegion> exclusive_regions(const std::vector<PlacedElement>& placed){
    std::vector<InputRegion> result;
    for(size_t i=0;i<placed.size();++i){
        const auto& e=placed[i];if(e.element.input!=InputPolicy::exclusive)continue;
        InputRegion region{e.element.id,e.clip,{e.clip}};
        for(size_t j=i+1;j<placed.size();++j){
            if(placed[j].element.input==InputPolicy::passthrough)continue;
            std::vector<Rect> next;for(auto r:region.fragments)subtract(r,placed[j].clip,next);
            if(next.size()>4096)throw std::runtime_error("Tela input region complexity limit exceeded");
            region.fragments=std::move(next);
        }
        if(!region.fragments.empty())result.push_back(std::move(region));
    }
    return result;
}
}
