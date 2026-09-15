// @spec SPEC-TL-SCENE-OVERLAY
#include <tela/scene_overlay.hpp>
#include <algorithm>
#include <array>

namespace tela {
namespace {
constexpr std::size_t shapes_per_canvas = 200; // Drawing accepts at most 256 commands
constexpr float max_layout_extent = 32768;     // Document layout dimension limit
constexpr float controls_width = 280, controls_margin = 12;
constexpr float label_padding = 2, outline_width = 1.5f, corner_radius = 3;
constexpr unsigned char fill_alpha = 48;
// Scenes are told apart by outline color; the palette repeats beyond six scenes.
constexpr std::array<Color, 6> palette{{
    {120, 200, 255, 255}, {255, 196, 90, 255}, {150, 230, 150, 255},
    {255, 140, 170, 255}, {200, 160, 255, 255}, {240, 240, 120, 255}}};

std::string scene_prefix(const std::string& scene_id) { return "scene-overlay/scene/" + scene_id; }
float layout_extent(float value) { return std::clamp(value, 1.0f, max_layout_extent); }

void declare_scene(Document& document, const SceneOverlay& overlay, const SceneOverlayScene& scene, Color color, Rect area) {
    const float scale = area.width / overlay.frame().width;
    std::vector<const SceneOverlayElement*> members;
    for(const auto& element : overlay.elements())
        if(element.scene_id == scene.id) members.push_back(&element);
    const Layout canvas_layout{.width = layout_extent(area.width), .height = layout_extent(area.height),
        .padding = 0, .positioned = true, .x = area.x, .y = area.y};
    for(std::size_t start = 0; start < members.size(); start += shapes_per_canvas) {
        Drawing drawing;
        const auto end = std::min(members.size(), start + shapes_per_canvas);
        for(std::size_t index = start; index < end; ++index) {
            const auto& b = members[index]->bounds;
            drawing.rectangle({b.x * scale, b.y * scale, b.width * scale, b.height * scale},
                {color.r, color.g, color.b, fill_alpha}, {color, outline_width}, corner_radius);
        }
        document.canvas(scene_prefix(scene.id) + "/shapes/" + std::to_string(start / shapes_per_canvas),
            std::move(drawing), canvas_layout);
    }
    for(const auto* element : members) {
        const auto& b = element->bounds;
        document.text(scene_prefix(scene.id) + "/label/" + element->id, element->label.empty() ? element->kind : element->label,
            {.width = layout_extent(b.width * scale), .padding = label_padding, .positioned = true,
             .x = area.x + b.x * scale, .y = area.y + b.y * scale});
    }
}
}

Rect scene_overlay_area(const SceneOverlayFrame& frame, Rect available) {
    if(available.width <= 0 || available.height <= 0 || frame.width <= 0 || frame.height <= 0)
        return {available.x, available.y, 0, 0};
    const float scale = std::min(available.width / frame.width, available.height / frame.height);
    const float width = frame.width * scale, height = frame.height * scale;
    return {available.x + (available.width - width) / 2, available.y + (available.height - height) / 2, width, height};
}

Document scene_overlay_document(const SceneOverlay& overlay, const Viewport& view,
    std::function<void(const std::string& scene_id)> toggle) {
    Document document;
    // A hidden or degenerate host declares nothing, matching the overlay lifecycle contract.
    if(!view.visible || view.width <= 0 || view.height <= 0 || !(view.dpi_scale > 0)) return document;
    const Rect area = scene_overlay_area(overlay.frame(), {0, 0, view.width / view.dpi_scale, view.height / view.dpi_scale});
    if(area.width <= 0 || area.height <= 0) return document;
    for(std::size_t index = 0; index < overlay.scenes().size(); ++index) {
        const auto& scene = overlay.scenes()[index];
        if(scene.visible) declare_scene(document, overlay, scene, palette[index % palette.size()], area);
    }
    Layout controls;
    controls.width = controls_width;
    controls.positioned = true;
    controls.x = controls_margin;
    controls.y = controls_margin;
    // Declared last so the toggles stay above the drawn placements.
    document.panel("scene-overlay/controls", [&] {
        document.text("scene-overlay/controls/title", overlay.frame().name.empty() ? "Scenes" : overlay.frame().name);
        for(const auto& scene : overlay.scenes()) {
            const std::string id = scene.id;
            const std::string name = scene.name.empty() ? id : scene.name;
            document.button("scene-overlay/toggle/" + id, (scene.visible ? "ON  " : "OFF ") + name, [toggle, id] { toggle(id); });
        }
    }, controls);
    return document;
}
}
