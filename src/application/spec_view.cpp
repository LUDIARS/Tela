// @spec SPEC-TL-SPEC-VIEW
#include <tela/spec_view.hpp>
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace tela {
namespace {
constexpr float max_extent = 100000; // Pf canvas coordinate range
constexpr std::size_t max_text_bytes = 4096;
// Group IDs and card codes become element ID segments, which the document bounds at 512 bytes.
constexpr std::size_t max_id_bytes = 256;
constexpr int max_spec_version = 1000000;

void require_valid(bool valid, const char* why) {
    if(!valid) throw std::invalid_argument(why);
}
void validate_text(const std::string& value) {
    require_valid(value.size() <= max_text_bytes && value.find_first_of("\r\n\0", 0, 3) == std::string::npos,
        "Spec view fields must be bounded single-line strings");
}
void validate_id(const std::string& value) {
    validate_text(value);
    require_valid(!value.empty() && value.size() <= max_id_bytes, "Spec view group IDs and card codes are required and bounded");
}
bool in_range(float value) { return std::isfinite(value) && std::abs(value) <= max_extent; }
void validate_bounds(const Rect& b) {
    require_valid(in_range(b.x) && in_range(b.y) && in_range(b.width) && in_range(b.height) && b.width > 0 && b.height > 0,
        "Spec view card bounds must be finite with a positive size");
}
}

SpecView::SpecView(SpecViewInfo info, std::vector<SpecViewGroup> groups, std::vector<SpecViewCard> cards)
    : info_(std::move(info)), groups_(std::move(groups)), cards_(std::move(cards)) {
    validate_text(info_.project);
    validate_text(info_.version);
    require_valid(in_range(info_.width) && in_range(info_.height) && info_.width > 0 && info_.height > 0,
        "Spec view size must be positive");
    require_valid(!groups_.empty() && groups_.size() <= spec_view_max_groups, "Spec view requires 1 to 32 groups");
    require_valid(cards_.size() <= spec_view_max_cards, "Spec view exceeds 256 cards");
    for(auto group = groups_.begin(); group != groups_.end(); ++group) {
        validate_id(group->id);
        validate_text(group->name);
        require_valid(std::none_of(groups_.begin(), group, [&](const auto& other) { return other.id == group->id; }),
            "Duplicate spec view group ID");
    }
    for(auto card = cards_.begin(); card != cards_.end(); ++card) {
        validate_id(card->group_id);
        validate_id(card->code);
        validate_text(card->title);
        validate_text(card->status);
        require_valid(card->version >= 0 && card->version <= max_spec_version, "Spec view card version must be a bounded count");
        validate_bounds(card->bounds);
        require_valid(std::any_of(groups_.begin(), groups_.end(), [&](const auto& group) { return group.id == card->group_id; }),
            "Spec view card references an unknown group");
        require_valid(std::none_of(cards_.begin(), card, [&](const auto& other) {
            return other.group_id == card->group_id && other.code == card->code; }), "Duplicate spec view card code");
    }
}

void SpecView::set_visible(const std::string& group_id, bool visible) {
    const auto group = std::find_if(groups_.begin(), groups_.end(), [&](const auto& item) { return item.id == group_id; });
    require_valid(group != groups_.end(), "Unknown spec view group");
    group->visible = visible;
}
}
