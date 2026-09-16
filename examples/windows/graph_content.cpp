// @spec Graph view
#include "graph_content.hpp"
#include <algorithm>

GraphContent::GraphContent(const std::string& path) : graph_(tela::load_graph(path)) {}

void GraphContent::toggle(const std::string& group_id) {
    const auto& groups = graph_.groups();
    const auto group = std::find_if(groups.begin(), groups.end(), [&](const auto& item) { return item.id == group_id; });
    if(group == groups.end()) return;
    graph_.set_visible(group_id, !group->visible);
    ++toggles_;
}

void GraphContent::refresh(tela::Runtime& runtime) {
    const auto& viewport = runtime.viewport();
    const Revision revision{viewport.width, viewport.height, viewport.dpi_scale, viewport.visible, toggles_};
    if(declared_ == revision) return;
    runtime.document(tela::graph_document(graph_, viewport, [this](const std::string& id) { toggle(id); }));
    declared_ = revision;
}
