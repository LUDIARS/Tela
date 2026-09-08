// @spec SPEC-TL-RUNTIME
#include <tela/layout.hpp>
#include <unordered_map>

namespace tela {
namespace {
class Arrangement {
public:
    Arrangement(const Document& d, const Theme& t) : theme(t) {
        for (const auto& e : d.elements()) children[e.parent].push_back(&e);
    }
    std::vector<PlacedElement> run(Rect viewport) {
        group("", viewport, viewport, Flow::column, 6);
        return std::move(result);
    }
private:
    const Theme& theme;
    std::unordered_map<std::string, std::vector<const Element*>> children;
    std::vector<PlacedElement> result;
    float naturalHeight(const Element& e) {
        if (e.layout.height) return e.layout.height;
        if (e.kind != ElementKind::panel) return theme.line_height + 2*e.layout.padding;
        float height = 0; unsigned count = 0;
        for (auto child : children[e.id]) {
            if (child->layout.positioned) continue;
            const float h = naturalHeight(*child);
            height = e.layout.flow == Flow::row ? std::max(height,h) : height+h;
            ++count;
        }
        if (e.layout.flow == Flow::column && count) height += (count-1)*e.layout.gap;
        return height + 2*e.layout.padding;
    }
    void group(const std::string& parent, Rect area, Rect clip, Flow flow, float gap) {
        float cursor = 0;
        for (auto e : children[parent]) {
            const auto& l = e->layout;
            float x = area.x + (l.positioned ? l.x : flow == Flow::row ? cursor : 0);
            float y = area.y + (l.positioned ? l.y : flow == Flow::column ? cursor : 0);
            const float available = std::max(0.f, area.x+area.width-x);
            Rect bounds{x,y,l.width ? l.width : available,naturalHeight(*e)};
            Rect ownClip = intersect(bounds, clip);
            result.push_back({*e,bounds,ownClip});
            if (e->kind == ElementKind::panel) {
                Rect inner{x+l.padding,y+l.padding,std::max(0.f,bounds.width-2*l.padding),
                           std::max(0.f,bounds.height-2*l.padding)};
                group(e->id,inner,intersect(inner,ownClip),l.flow,l.gap);
            }
            if (!l.positioned) cursor += (flow == Flow::row ? bounds.width : bounds.height)+gap;
        }
    }
};
}
std::vector<PlacedElement> arrange(const Document& d, float w, float h, const Theme& theme) {
    return Arrangement(d,theme).run({0,0,w,h});
}
}
