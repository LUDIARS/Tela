// @implements SPEC-TL-SCENE-OVERLAY
// @spec Scene overlay
#pragma once
#include <tela/document.hpp>
#include <tela/host_contract.hpp>
#include <cstddef>
#include <filesystem>
#include <functional>
#include <string>
#include <vector>

namespace tela {
struct SceneOverlayFrame {
    std::string name;
    float width{}, height{};
    bool operator==(const SceneOverlayFrame&) const = default;
};
struct SceneOverlayScene {
    std::string id, name;
    bool visible{};
    bool operator==(const SceneOverlayScene&) const = default;
};
struct SceneOverlayElement {
    std::string scene_id, id, kind, label;
    Rect bounds; // exported frame coordinates
    bool operator==(const SceneOverlayElement&) const = default;
};
inline constexpr std::size_t scene_overlay_max_scenes = 32;
inline constexpr std::size_t scene_overlay_max_elements = 1024;

// Application data exported by Pf: one frame, scenes bottom to top and their placeholder elements.
// It is a read-only view of Pf data, not a second editable source of the scene.
class SceneOverlay {
public:
    SceneOverlay(SceneOverlayFrame frame, std::vector<SceneOverlayScene> scenes,
                 std::vector<SceneOverlayElement> elements);
    const SceneOverlayFrame& frame() const noexcept { return frame_; }
    const std::vector<SceneOverlayScene>& scenes() const noexcept { return scenes_; }
    const std::vector<SceneOverlayElement>& elements() const noexcept { return elements_; }
    // Session state only; the loaded file is never rewritten.
    void set_visible(const std::string& scene_id, bool visible);
private:
    SceneOverlayFrame frame_;
    std::vector<SceneOverlayScene> scenes_;
    std::vector<SceneOverlayElement> elements_;
};

// Reads `TELA_SCENE_OVERLAY 1`. An invalid file throws and yields no partial overlay.
SceneOverlay load_scene_overlay(const std::filesystem::path&);

// Fits the exported frame inside a logical area, keeping its aspect ratio and centering it.
Rect scene_overlay_area(const SceneOverlayFrame&, Rect available);

// Visible scenes draw outlined elements with labels; every scene keeps a toggle button.
Document scene_overlay_document(const SceneOverlay&, const Viewport&,
    std::function<void(const std::string& scene_id)> toggle);
}
