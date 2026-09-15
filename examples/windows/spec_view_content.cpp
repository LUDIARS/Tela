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

void SpecViewContent::refresh(tela::Runtime& runtime) {
    const auto& viewport = runtime.viewport();
    const Revision revision{viewport.width, viewport.height, viewport.dpi_scale, viewport.visible, toggles_};
    if(declared_ == revision) return;
    runtime.document(tela::spec_view_document(view_, viewport, [this](const std::string& id) { toggle(id); }));
    declared_ = revision;
}
