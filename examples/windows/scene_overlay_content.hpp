#pragma once
#include <tela/runtime.hpp>
#include <tela/scene_overlay.hpp>
#include <cstdint>
#include <optional>
#include <string>
// @spec Scene overlay
// Owns the loaded Pf scene overlay. Toggle actions only change state; refresh redeclares
// when something the declaration reads has changed.
class SceneOverlayContent {
public:
    explicit SceneOverlayContent(const std::string& path);
    void refresh(tela::Runtime& runtime);
private:
    void toggle(const std::string& scene_id);
    // Keyed on declaration inputs rather than the viewport revision, which resets on disconnect.
    struct Revision {
        int width, height;
        float dpi_scale;
        bool visible;
        std::uint64_t toggles;
        bool operator==(const Revision&) const = default;
    };
    tela::SceneOverlay overlay_;
    std::uint64_t toggles_{};
    std::optional<Revision> declared_;
};
