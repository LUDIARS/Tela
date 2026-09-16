// @implements SPEC-TL-GRAPH
// @spec Graph view
#pragma once
#include <tela/document.hpp>
#include <tela/host_contract.hpp>
#include <cstddef>
#include <filesystem>
#include <functional>
#include <string>
#include <vector>

namespace tela {
struct GraphInfo {
    std::string project, title;
    float width{}, height{};
    bool operator==(const GraphInfo&) const = default;
};
struct GraphGroup {
    std::string id, name;
    bool visible{};
    Color color;
    bool operator==(const GraphGroup&) const = default;
};
struct GraphNode {
    std::string group_id, id, label;
    Rect bounds; // exported graph coordinates
    bool operator==(const GraphNode&) const = default;
};
struct GraphEdge {
    std::string id, from, to;
    bool dashed{};
    std::vector<Point> points; // exported graph coordinates, already routed by Pf
    bool operator==(const GraphEdge&) const = default;
};
inline constexpr std::size_t graph_max_groups = 8;
inline constexpr std::size_t graph_max_nodes = 128;
inline constexpr std::size_t graph_max_edges = 256;
inline constexpr std::size_t graph_max_points = 4096;

// Application data exported by Pf: the classification columns, their nodes and the routed
// relations between them. It is a read-only picture of Pf data; Tela lays nothing out.
class Graph {
public:
    Graph(GraphInfo info, std::vector<GraphGroup> groups, std::vector<GraphNode> nodes, std::vector<GraphEdge> edges);
    const GraphInfo& info() const noexcept { return info_; }
    const std::vector<GraphGroup>& groups() const noexcept { return groups_; }
    const std::vector<GraphNode>& nodes() const noexcept { return nodes_; }
    const std::vector<GraphEdge>& edges() const noexcept { return edges_; }
    // Session state only; the loaded file is never rewritten.
    void set_visible(const std::string& group_id, bool visible);
private:
    GraphInfo info_;
    std::vector<GraphGroup> groups_;
    std::vector<GraphNode> nodes_;
    std::vector<GraphEdge> edges_;
};

// Reads `TELA_GRAPH 1`. An invalid file throws and yields no partial graph.
Graph load_graph(const std::filesystem::path&);

// Fits the exported graph inside a logical area, keeping its aspect ratio and centering it.
Rect graph_area(const GraphInfo&, Rect available);

// Visible groups draw their nodes and the relations whose both ends are visible.
Document graph_document(const Graph&, const Viewport&, std::function<void(const std::string& group_id)> toggle);
}
