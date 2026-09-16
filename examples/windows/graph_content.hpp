#pragma once
#include <tela/graph.hpp>
#include <tela/runtime.hpp>
#include <cstdint>
#include <optional>
#include <string>
// @spec Graph view
// Owns the loaded Pf relation graph. Toggle actions only change state; refresh redeclares
// when something the declaration reads has changed.
class GraphContent {
public:
    explicit GraphContent(const std::string& path);
    void refresh(tela::Runtime& runtime);
    // The exported graph has its own size; placements outside the host draw it at that size.
    // @implements SPEC-TL-PLACEMENT
    tela::Rect natural_bounds() const noexcept { return {0, 0, graph_.info().width, graph_.info().height}; }
private:
    void toggle(const std::string& group_id);
    // Keyed on declaration inputs rather than the viewport revision, which resets on disconnect.
    struct Revision {
        int width, height;
        float dpi_scale;
        bool visible;
        std::uint64_t toggles;
        bool operator==(const Revision&) const = default;
    };
    tela::Graph graph_;
    std::uint64_t toggles_{};
    std::optional<Revision> declared_;
};
