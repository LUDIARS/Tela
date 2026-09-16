// @implements SPEC-TL-RUNTIME
// @spec Runtime
#pragma once
#include <algorithm>

namespace tela {
struct Rect {
    float x{}, y{}, width{}, height{};
    bool contains(float px, float py) const noexcept {
        return px >= x && py >= y && px < x + width && py < y + height;
    }
    bool operator==(const Rect&) const = default;
};
inline Rect intersect(Rect a, Rect b) {
    const auto x = std::max(a.x, b.x), y = std::max(a.y, b.y);
    return {x, y, std::max(0.0f, std::min(a.x+a.width, b.x+b.width)-x),
                  std::max(0.0f, std::min(a.y+a.height, b.y+b.height)-y)};
}
// Layout dimensions are logical pixels. Hosts apply DPI once at the boundary.
enum class Flow { column, row };
struct Layout {
    float width{}, height{}; // zero = available width / content height
    float padding{8}, gap{6};
    Flow flow{Flow::column};
    bool positioned{};
    float x{}, y{};
    // Text rows the box reserves. The renderer wraps inside the width, so a single row
    // silently drops the rest; raising this is how a caller asks for wrapped text.
    unsigned lines{1};
    bool operator==(const Layout&) const = default;
};
struct Color {
    unsigned char r{}, g{}, b{}, a{255};
    bool operator==(const Color&) const = default;
};
struct Theme {
    Color panel{25,30,42,220}, text{245,246,250,255};
    Color button{52,79,120,245}, pressed{90,136,194,255};
    float font_size{16}, line_height{24};
    bool operator==(const Theme&) const = default;
};
}
