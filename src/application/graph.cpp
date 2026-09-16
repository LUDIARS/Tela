// @spec SPEC-TL-GRAPH
#include <tela/graph.hpp>
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace tela {
namespace {
constexpr float max_extent = 100000; // Pf canvas coordinate range
constexpr std::size_t max_text_bytes = 4096;
// Group IDs and node IDs become element ID segments, which the document bounds at 512 bytes.
constexpr std::size_t max_id_bytes = 256;

void require_valid(bool valid, const char* why) {
    if(!valid) throw std::invalid_argument(why);
}
void validate_text(const std::string& value) {
    require_valid(value.size() <= max_text_bytes && value.find_first_of("\r\n\0", 0, 3) == std::string::npos,
        "Graph fields must be bounded single-line strings");
}
void validate_id(const std::string& value) {
    validate_text(value);
    require_valid(!value.empty() && value.size() <= max_id_bytes, "Graph IDs are required and bounded");
}
bool in_range(float value) { return std::isfinite(value) && std::abs(value) <= max_extent; }
}

Graph::Graph(GraphInfo info, std::vector<GraphGroup> groups, std::vector<GraphNode> nodes, std::vector<GraphEdge> edges)
    : info_(std::move(info)), groups_(std::move(groups)), nodes_(std::move(nodes)), edges_(std::move(edges)) {
    validate_text(info_.project);
    validate_text(info_.title);
    require_valid(in_range(info_.width) && in_range(info_.height) && info_.width > 0 && info_.height > 0,
        "Graph size must be positive");
    require_valid(!groups_.empty() && groups_.size() <= graph_max_groups, "Graph requires 1 to 8 groups");
    require_valid(!nodes_.empty() && nodes_.size() <= graph_max_nodes, "Graph requires 1 to 128 nodes");
    require_valid(edges_.size() <= graph_max_edges, "Graph exceeds 256 edges");
    for(auto group = groups_.begin(); group != groups_.end(); ++group) {
        validate_id(group->id);
        validate_text(group->name);
        require_valid(std::none_of(groups_.begin(), group, [&](const auto& other) { return other.id == group->id; }),
            "Duplicate graph group ID");
    }
    for(auto node = nodes_.begin(); node != nodes_.end(); ++node) {
        validate_id(node->group_id);
        validate_id(node->id);
        validate_text(node->label);
        const auto& b = node->bounds;
        require_valid(in_range(b.x) && in_range(b.y) && in_range(b.width) && in_range(b.height)
            && b.width > 0 && b.height > 0, "Graph node bounds must be finite with a positive size");
        require_valid(std::any_of(groups_.begin(), groups_.end(), [&](const auto& group) { return group.id == node->group_id; }),
            "Graph node references an unknown group");
        require_valid(std::none_of(nodes_.begin(), node, [&](const auto& other) { return other.id == node->id; }),
            "Duplicate graph node ID");
    }
    std::size_t points = 0;
    for(auto edge = edges_.begin(); edge != edges_.end(); ++edge) {
        validate_id(edge->id);
        validate_id(edge->from);
        validate_id(edge->to);
        require_valid(std::none_of(edges_.begin(), edge, [&](const auto& other) { return other.id == edge->id; }),
            "Duplicate graph edge ID");
        // A relation Tela cannot place would be drawn from nowhere, so the file is rejected.
        for(const auto* end : {&edge->from, &edge->to})
            require_valid(std::any_of(nodes_.begin(), nodes_.end(), [&](const auto& node) { return node.id == *end; }),
                "Graph edge references an unknown node");
        require_valid(edge->points.size() >= 2, "Graph edge needs at least two route points");
        points += edge->points.size();
        for(const auto& point : edge->points)
            require_valid(in_range(point.x) && in_range(point.y), "Graph route points must be finite");
    }
    require_valid(points <= graph_max_points, "Graph exceeds 4096 route points");
}

void Graph::set_visible(const std::string& group_id, bool visible) {
    const auto group = std::find_if(groups_.begin(), groups_.end(), [&](const auto& item) { return item.id == group_id; });
    require_valid(group != groups_.end(), "Unknown graph group");
    group->visible = visible;
}
}
