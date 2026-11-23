// @spec SPEC-TL-RUNTIME
#include <tela/layout.hpp>
#include <algorithm>

namespace tela {
namespace {
bool container(ElementKind kind) { return kind == ElementKind::panel || kind == ElementKind::scroll; }
class Arrangement {
public:
    Arrangement(const Document& d, const Theme& t, const ScrollOffsets& o) : theme(t), offsets(o) {
        for (const auto& e : d.elements()) children[e.parent].push_back(&e);
    }
    std::vector<PlacedElement> run(Rect viewport) {
        group("", viewport, viewport, Flow::column, 6);
        return std::move(result);
    }
private:
    const Theme& theme;
    const ScrollOffsets& offsets;
    std::unordered_map<std::string, std::vector<const Element*>> children;
    std::vector<PlacedElement> result;
    // Height of a container's flowing children, without its own padding.
    float contentHeight(const Element& e) {
        float height = 0; unsigned count = 0;
        for (auto child : children[e.id]) {
            if (child->layout.positioned) continue;
            const float h = naturalHeight(*child);
            height = e.layout.flow == Flow::row ? std::max(height,h) : height+h;
            ++count;
        }
        if (e.layout.flow == Flow::column && count) height += (count-1)*e.layout.gap;
        return height;
    }
    float naturalHeight(const Element& e) {
        if (e.layout.height) return e.layout.height;
        if (e.kind == ElementKind::separator) return 1 + 2*e.layout.padding;
        // A scroll without a height fills its parent when placed; alone it asks for nothing.
        if (e.kind == ElementKind::scroll) return 2*e.layout.padding;
        if (!container(e.kind)) return theme.line_height*e.layout.text_scale*std::max(1u,e.layout.lines) + 2*e.layout.padding;
        return contentHeight(e) + 2*e.layout.padding;
    }
    void group(const std::string& parent, Rect area, Rect clip, Flow flow, float gap) {
        float cursor = 0;
        for (auto e : children[parent]) {
            const auto& l = e->layout;
            float x = area.x + (l.positioned ? l.x : flow == Flow::row ? cursor : 0);
            float y = area.y + (l.positioned ? l.y : flow == Flow::column ? cursor : 0);
            const float available = std::max(0.f, area.x+area.width-x);
            float height = naturalHeight(*e);
            if (e->kind == ElementKind::scroll && !l.height) height = std::max(height, area.y+area.height-y);
            Rect bounds{x,y,l.width ? l.width : available,height};
            Rect ownClip = intersect(bounds, clip);
            result.push_back({*e,bounds,ownClip});
            if (container(e->kind)) {
                Rect inner{x+l.padding,y+l.padding,std::max(0.f,bounds.width-2*l.padding),
                           std::max(0.f,bounds.height-2*l.padding)};
                Rect content = inner;
                if (e->kind == ElementKind::scroll) {
                    const float total = contentHeight(*e);
                    const auto wanted = offsets.find(e->id);
                    const float offset = std::clamp(wanted == offsets.end() ? 0.f : wanted->second,
                                                    0.f, std::max(0.f, total-inner.height));
                    auto& placed = result.back();
                    placed.scroll = offset; placed.content_height = total;
                    content.y -= offset; content.height = std::max(inner.height, total);
                }
                group(e->id,content,intersect(inner,ownClip),l.flow,l.gap);
            }
            if (!l.positioned) cursor += (flow == Flow::row ? bounds.width : bounds.height)+gap;
        }
    }
};
}
std::vector<PlacedElement> arrange(const Document& d, float w, float h, const Theme& theme,
                                   const ScrollOffsets& offsets) {
    return Arrangement(d,theme,offsets).run({0,0,w,h});
}
}