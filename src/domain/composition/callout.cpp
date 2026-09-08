// @spec Overlay drawing
#include <tela/callout.hpp>
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace tela {
namespace {
void validate(const Callout& c,Rect area) {
    for(float v:{c.anchor.x,c.anchor.y,c.offset.x,c.offset.y,c.width,c.height,c.margin,c.radius,
        area.x,area.y,area.width,area.height})
        // Logical extents reach viewport pixels / minimum dpi_scale (16384 / .25).
        if(!std::isfinite(v)||std::abs(v)>65536) throw std::invalid_argument("Invalid callout geometry");
    if(c.width<=0||c.height<=0||c.margin<0||c.radius<0||area.width<0||area.height<0)
        throw std::invalid_argument("Invalid callout dimensions");
}
float place_axis(float anchor,float offset,float size,float start,float extent,float margin) {
    float desired=anchor+offset;
    if(desired+size>start+extent-margin) desired=anchor-std::abs(offset)-size;
    if(desired<start+margin) desired=anchor+std::abs(offset);
    return std::clamp(desired,start+margin,start+extent-margin-size);
}
Point attach(Point anchor,Rect box) {
    const Point center{box.x+box.width/2,box.y+box.height/2};
    const float dx=anchor.x-center.x,dy=anchor.y-center.y;
    if(dx==0&&dy==0) return {box.x,center.y};
    const float factor=std::max(std::abs(dx)/(box.width/2),std::abs(dy)/(box.height/2));
    return {center.x+dx/factor,center.y+dy/factor};
}
}
std::optional<CalloutPlacement> place_callout(const Callout& c,Rect area) {
    validate(c,area);
    if(!c.visible||!area.contains(c.anchor.x,c.anchor.y)) return std::nullopt;
    if(c.width+2*c.margin>area.width||c.height+2*c.margin>area.height) return std::nullopt;
    Rect label{place_axis(c.anchor.x,c.offset.x,c.width,area.x,area.width,c.margin),
        place_axis(c.anchor.y,c.offset.y,c.height,area.y,area.height,c.margin),c.width,c.height};
    return CalloutPlacement{label,c.anchor,attach(c.anchor,label)};
}
void callout(Document& document,const std::string& id,const std::string& text,
    const Callout& config,Rect area) {
    const auto placed=place_callout(config,area);
    if(!placed) return;
    Drawing drawing;
    const Point from{placed->anchor.x-area.x,placed->anchor.y-area.y};
    const Point to{placed->attachment.x-area.x,placed->attachment.y-area.y};
    drawing.line(from,to,config.leader);
    const auto& label=placed->label;
    drawing.rectangle({label.x-area.x,label.y-area.y,label.width,label.height},config.background,
        {{0,0,0,0},0},config.radius);
    // Commit the composite declaration atomically if either stable ID conflicts.
    Document next=document;
    next.canvas(id+".drawing",std::move(drawing),{.width=area.width,.height=area.height,.padding=0,
        .positioned=true,.x=area.x,.y=area.y});
    next.text(id+".label",text,{.width=label.width,.height=label.height,
        .positioned=true,.x=label.x,.y=label.y});
    document=std::move(next);
}
}
