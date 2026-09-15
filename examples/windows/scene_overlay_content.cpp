// @spec Scene overlay
#include "scene_overlay_content.hpp"
#include <algorithm>

SceneOverlayContent::SceneOverlayContent(const std::string& path) : overlay_(tela::load_scene_overlay(path)) {}

void SceneOverlayContent::toggle(const std::string& scene_id) {
    const auto& scenes = overlay_.scenes();
    const auto scene = std::find_if(scenes.begin(), scenes.end(), [&](const auto& item) { return item.id == scene_id; });
    if(scene == scenes.end()) return;
    overlay_.set_visible(scene_id, !scene->visible);
    ++toggles_;
}

void SceneOverlayContent::refresh(tela::Runtime& runtime) {
    const auto& view = runtime.viewport();
    const Revision revision{view.width, view.height, view.dpi_scale, view.visible, toggles_};
    if(declared_ == revision) return;
    runtime.document(tela::scene_overlay_document(overlay_, view, [this](const std::string& id) { toggle(id); }));
    declared_ = revision;
}
