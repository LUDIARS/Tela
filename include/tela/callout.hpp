#pragma once
#include <tela/document.hpp>
#include <optional>

namespace tela {
struct Callout {
    Point anchor;
    Point offset{96,-110};
    float width{150},height{36},margin{12},radius{6};
    bool visible{true};
    Color background{25,30,42,230};
    Stroke leader{{240,240,250,255},1.5f};
};
struct CalloutPlacement { Rect label; Point anchor, attachment; };
// @spec Overlay drawing
// Coordinates are logical and relative to the current parent content area.
std::optional<CalloutPlacement> place_callout(const Callout&,Rect visible_area);
void callout(Document&,const std::string& id,const std::string& label,
             const Callout&,Rect visible_area);
}
