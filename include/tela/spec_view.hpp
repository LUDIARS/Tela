// @implements SPEC-TL-SPEC-VIEW
// @spec Spec view
#pragma once
#include <tela/document.hpp>
#include <tela/host_contract.hpp>
#include <cstddef>
#include <filesystem>
#include <functional>
#include <string>
#include <vector>

namespace tela {
struct SpecViewInfo {
    std::string project, version;
    float width{}, height{};
    bool operator==(const SpecViewInfo&) const = default;
};
struct SpecViewGroup {
    std::string id, name;
    bool visible{};
    bool operator==(const SpecViewGroup&) const = default;
};
struct SpecViewCard {
    std::string group_id, code, title, status;
    int version{};
    Rect bounds; // exported view coordinates
    bool operator==(const SpecViewCard&) const = default;
};
inline constexpr std::size_t spec_view_max_groups = 32;
inline constexpr std::size_t spec_view_max_cards = 256;
// Logical width of the left-hand control strip; placements beside a host add it to the picture.
inline constexpr float spec_view_controls_width = 280;
enum class SpecViewAction { previous, next, first };

// Application data exported by Pf: one view extent, the groups of a chosen axis and their
// specification cards. It is a read-only picture of Pf data, not a second editable source
// of the specification.
class SpecView {
public:
    SpecView(SpecViewInfo info, std::vector<SpecViewGroup> groups, std::vector<SpecViewCard> cards);
    const SpecViewInfo& info() const noexcept { return info_; }
    const std::vector<SpecViewGroup>& groups() const noexcept { return groups_; }
    const std::vector<SpecViewCard>& cards() const noexcept { return cards_; }
    // Session state only; the loaded file is never rewritten.
    void set_visible(const std::string& group_id, bool visible);
    // Top of the shown part in exported view coordinates; always 0 or a card's top.
    float scroll() const noexcept { return scroll_; }
    // Pages by whole cards: the next page starts at the first card not fully shown, so none
    // is skipped. `page` is the exported height visible at once (spec_view_page).
    void navigate(SpecViewAction, float page);
private:
    SpecViewInfo info_;
    std::vector<SpecViewGroup> groups_;
    std::vector<SpecViewCard> cards_;
    float scroll_{};
};

// Reads `TELA_SPEC_VIEW 1`. An invalid file throws and yields no partial view.
SpecView load_spec_view(const std::filesystem::path&);

// Places the exported view in a logical area keeping its aspect ratio. A view that fits at
// scale 1 or more is fitted whole and centered. Otherwise it fits the width, never above
// scale 1, and a taller result starts at the area's top to be read page by page.
Rect spec_view_area(const SpecViewInfo&, Rect available);

// Exported height visible at once in the viewport; the whole height when nothing scrolls.
float spec_view_page(const SpecViewInfo&, const Viewport&);

// Visible groups draw outlined cards with their code and title; every group keeps a toggle button.
// A view taller than the viewport shows only whole cards from its scroll position and adds
// page controls.
Document spec_view_document(const SpecView&, const Viewport&,
    std::function<void(const std::string& group_id)> toggle, const Theme& theme = {},
    std::function<void(SpecViewAction)> navigate = {});
}
