// @spec SPEC-TL-SCENE-OVERLAY
#include <tela/scene_overlay.hpp>
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace tela {
namespace {
constexpr float max_extent = 100000; // Pf canvas coordinate range
constexpr std::size_t max_field_bytes = 4096;

void require(bool valid, const char* why) {
    if(!valid) throw std::invalid_argument(why);
}
void validate_field(const std::string& value, bool required) {
    require(value.size() <= max_field_bytes && value.find_first_of("\r\n\0", 0, 3) == std::string::npos,
        "Scene overlay fields must be bounded single-line strings");
    require(!required || !value.empty(), "Scene overlay IDs and kinds are required");
}
bool in_range(float value) { return std::isfinite(value) && std::abs(value) <= max_extent; }
void validate_bounds(const Rect& b) {
    require(in_range(b.x) && in_range(b.y) && in_range(b.width) && in_range(b.height) && b.width > 0 && b.height > 0,
        "Scene overlay element bounds must be finite with a positive size");
}
}

SceneOverlay::SceneOverlay(SceneOverlayFrame frame, std::vector<SceneOverlayScene> scenes,
                           std::vector<SceneOverlayElement> elements)
    : frame_(std::move(frame)), scenes_(std::move(scenes)), elements_(std::move(elements)) {
    validate_field(frame_.name, false);
    require(in_range(frame_.width) && in_range(frame_.height) && frame_.width > 0 && frame_.height > 0,
        "Scene overlay frame size must be positive");
    require(!scenes_.empty() && scenes_.size() <= scene_overlay_max_scenes, "Scene overlay requires 1 to 32 scenes");
    require(elements_.size() <= scene_overlay_max_elements, "Scene overlay exceeds 1024 elements");
    for(auto scene = scenes_.begin(); scene != scenes_.end(); ++scene) {
        validate_field(scene->id, true);
        validate_field(scene->name, false);
        require(std::none_of(scenes_.begin(), scene, [&](const auto& other) { return other.id == scene->id; }),
            "Duplicate scene overlay scene ID");
    }
    for(auto element = elements_.begin(); element != elements_.end(); ++element) {
        for(const auto* field : {&element->scene_id, &element->id, &element->kind}) validate_field(*field, true);
        validate_field(element->label, false);
        validate_bounds(element->bounds);
        require(std::any_of(scenes_.begin(), scenes_.end(), [&](const auto& scene) { return scene.id == element->scene_id; }),
            "Scene overlay element references an unknown scene");
        require(std::none_of(elements_.begin(), element, [&](const auto& other) {
            return other.scene_id == element->scene_id && other.id == element->id; }), "Duplicate scene overlay element ID");
    }
}

void SceneOverlay::set_visible(const std::string& scene_id, bool visible) {
    const auto scene = std::find_if(scenes_.begin(), scenes_.end(), [&](const auto& item) { return item.id == scene_id; });
    require(scene != scenes_.end(), "Unknown scene overlay scene");
    scene->visible = visible;
}
}
