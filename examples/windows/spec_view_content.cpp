// @spec Spec view
#include "spec_view_content.hpp"
#include <algorithm>

SpecViewContent::SpecViewContent(const std::string& path) : view_(tela::load_spec_view(path)) {}

void SpecViewContent::toggle(const std::string& group_id) {
    const auto& groups = view_.groups();
    const auto group = std::find_if(groups.begin(), groups.end(), [&](const auto& item) { return item.id == group_id; });
    if(group == groups.end()) return;
    view_.set_visible(group_id, !group->visible);
    ++toggles_;
}

void SpecViewContent::navigate(tela::SpecViewAction action) {
    // Pages by what the user saw when pressing; a later resize keeps the same top card.
    view_.navigate(action, page_);
}

void SpecViewContent::refresh(tela::Runtime& runtime) {
    const auto& viewport = runtime.viewport();
    const Revision revision{viewport.width, viewport.height, viewport.dpi_scale, viewport.visible, toggles_,
        view_.scroll(), runtime.theme()};
    if(declared_ == revision) return;
    page_ = tela::spec_view_page(view_.info(), viewport);
    runtime.document(tela::spec_view_document(view_, viewport, [this](const std::string& id) { toggle(id); }, runtime.theme(),
        [this](tela::SpecViewAction action) { navigate(action); }));
    declared_ = revision;
}
