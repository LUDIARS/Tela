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
private:
    SpecViewInfo info_;
    std::vector<SpecViewGroup> groups_;
    std::vector<SpecViewCard> cards_;
};

// Reads `TELA_SPEC_VIEW 1`. An invalid file throws and yields no partial view.
SpecView load_spec_view(const std::filesystem::path&);

// Fits the exported view inside a logical area, keeping its aspect ratio and centering it.
Rect spec_view_area(const SpecViewInfo&, Rect available);

// Visible groups draw outlined cards with their code and title; every group keeps a toggle button.
Document spec_view_document(const SpecView&, const Viewport&,
    std::function<void(const std::string& group_id)> toggle);
}
