// @spec SPEC-TL-GRAPH
#include <tela/graph.hpp>
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <optional>
#include <sstream>
#include <stdexcept>

namespace tela {
namespace {
constexpr std::uintmax_t max_file_bytes = 4 * 1024 * 1024;

void finish_row(std::istringstream& row) {
    row >> std::ws;
    if(!row.eof()) throw std::invalid_argument("Trailing graph data");
}
bool read_flag(std::istringstream& row) {
    int flag = -1;
    if(!(row >> flag) || (flag != 0 && flag != 1)) throw std::invalid_argument("Graph flags must be 0 or 1");
    return flag == 1;
}
unsigned char read_channel(std::istringstream& row) {
    int value = -1;
    if(!(row >> value) || value < 0 || value > 255) throw std::invalid_argument("Graph colours must be 0..255");
    return static_cast<unsigned char>(value);
}
GraphInfo read_graph(std::istringstream& row) {
    GraphInfo info;
    if(!(row >> std::quoted(info.project) >> std::quoted(info.title) >> info.width >> info.height))
        throw std::invalid_argument("Invalid graph header");
    finish_row(row);
    return info;
}
GraphGroup read_group(std::istringstream& row) {
    GraphGroup group;
    if(!(row >> std::quoted(group.id) >> std::quoted(group.name))) throw std::invalid_argument("Invalid graph group");
    group.visible = read_flag(row);
    group.color = {read_channel(row), read_channel(row), read_channel(row), 255};
    finish_row(row);
    return group;
}
GraphNode read_node(std::istringstream& row) {
    GraphNode node;
    auto& b = node.bounds;
    if(!(row >> std::quoted(node.group_id) >> std::quoted(node.id) >> std::quoted(node.label)
             >> b.x >> b.y >> b.width >> b.height))
        throw std::invalid_argument("Invalid graph node");
    finish_row(row);
    return node;
}
GraphEdge read_edge(std::istringstream& row) {
    GraphEdge edge;
    if(!(row >> std::quoted(edge.id) >> std::quoted(edge.from) >> std::quoted(edge.to)))
        throw std::invalid_argument("Invalid graph edge");
    edge.dashed = read_flag(row);
    finish_row(row);
    return edge;
}
void read_point(std::istringstream& row, std::vector<GraphEdge>& edges) {
    std::string id;
    Point point;
    if(!(row >> std::quoted(id) >> point.x >> point.y)) throw std::invalid_argument("Invalid graph route point");
    finish_row(row);
    const auto edge = std::find_if(edges.begin(), edges.end(), [&](const auto& item) { return item.id == id; });
    // Points follow their edge; one that does not would silently bend another relation.
    if(edge == edges.end()) throw std::invalid_argument("Graph route point references an unknown edge");
    edge->points.push_back(point);
}
}

Graph load_graph(const std::filesystem::path& path) {
    if(std::filesystem::file_size(path) > max_file_bytes) throw std::invalid_argument("Graph file exceeds 4 MiB");
    std::ifstream input(path, std::ios::binary);
    std::string line;
    if(!input || !std::getline(input, line) || line != "TELA_GRAPH 1")
        throw std::invalid_argument("Unsupported graph file");
    std::optional<GraphInfo> info;
    std::vector<GraphGroup> groups;
    std::vector<GraphNode> nodes;
    std::vector<GraphEdge> edges;
    std::size_t points = 0;
    while(std::getline(input, line)) {
        if(line.empty()) continue;
        std::istringstream row(line);
        std::string record;
        row >> record;
        if(record == "graph") {
            if(info) throw std::invalid_argument("Duplicate graph header");
            info = read_graph(row);
        } else if(record == "group") {
            groups.push_back(read_group(row));
        } else if(record == "node") {
            nodes.push_back(read_node(row));
        } else if(record == "edge") {
            edges.push_back(read_edge(row));
        } else if(record == "point") {
            read_point(row, edges);
            ++points;
        } else {
            throw std::invalid_argument("Unknown graph record");
        }
        // Stop reading an oversized file early; the model repeats the limits for in-memory callers.
        if(groups.size() > graph_max_groups || nodes.size() > graph_max_nodes
           || edges.size() > graph_max_edges || points > graph_max_points)
            throw std::invalid_argument("Graph limit exceeded");
    }
    if(input.bad()) throw std::runtime_error("Graph read failed");
    if(!info) throw std::invalid_argument("Graph header is required");
    return Graph(std::move(*info), std::move(groups), std::move(nodes), std::move(edges));
}
}
