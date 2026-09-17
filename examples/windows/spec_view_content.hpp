#pragma once
#include <tela/runtime.hpp>
#include <tela/spec_view.hpp>
#include <cstdint>
#include <optional>
#include <string>
// @spec Spec view
// Owns the loaded Pf spec view. Toggle actions only change state; refresh redeclares
// when something the declaration reads has changed.
class SpecViewContent {
public:
    explicit SpecViewContent(const std::string& path);
    void refresh(tela::Runtime& runtime);
    // The exported view has its own size; placements outside the host draw it at that size.
    // @implements SPEC-TL-PLACEMENT
    tela::Rect natural_bounds() const noexcept { return {0, 0, view_.info().width, view_.info().height}; }
private:
    void toggle(const std::string& group_id);
    // Keyed on declaration inputs rather than the viewport revision, which resets on disconnect.
    struct Revision {
        int width, height;
        float dpi_scale;
        bool visible;
        std::uint64_t toggles;
        tela::Theme theme;
        bool operator==(const Revision&) const = default;
    };
    tela::SpecView view_;
    std::uint64_t toggles_{};
    std::optional<Revision> declared_;
};
