// @spec SPEC-TL-GRAPH
#include <tela/graph.hpp>
#include <algorithm>
#include <cmath>

namespace tela {
namespace {
constexpr std::size_t shapes_per_canvas = 200; // Drawing accepts at most 256 commands
constexpr float max_layout_extent = 32768;     // Document layout dimension limit
constexpr float controls_width = 280, controls_margin = 12;
constexpr float node_padding = 6, outline_width = 2, corner_radius = 8;
constexpr float heading_offset = 30, label_offset = 6;
constexpr float assumed_line_height = 24; // Pf sizes nodes for the default 16 px theme
// Pf draws the relations in a neutral grey so the classification colours stay on the nodes.
constexpr Color edge_color{102, 112, 133, 255};
constexpr float edge_width = 2, edge_dash = 6, edge_gap = 4;
// 関係は有向なので終端に矢じりを描く。経路は Pf が決めたものなので、向きはその最後の
// 区間から取る。矢じりは実線で描き、破線の関係でも向きが読めるようにする。
constexpr float arrow_length = 12, arrow_spread = 0.42f; // rad, およそ 24 度
// 1 つの関係は経路と矢じりの 2 図形になるので、1 キャンバス 100 本までに分ける。
constexpr std::size_t edges_per_canvas = 100;

std::string group_prefix(const std::string& group_id) { return "graph/group/" + group_id; }
float layout_extent(float value) { return std::clamp(value, 1.0f, max_layout_extent); }

const GraphNode* find_node(const Graph& graph, const std::string& id) {
    const auto& nodes = graph.nodes();
    const auto found = std::find_if(nodes.begin(), nodes.end(), [&](const auto& node) { return node.id == id; });
    return found == nodes.end() ? nullptr : &*found;
}
bool group_visible(const Graph& graph, const std::string& group_id) {
    const auto& groups = graph.groups();
    const auto found = std::find_if(groups.begin(), groups.end(), [&](const auto& group) { return group.id == group_id; });
    return found != groups.end() && found->visible;
}

void declare_nodes(Document& document, const Graph& graph, const GraphGroup& group, Rect area, float scale) {
    std::vector<const GraphNode*> members;
    for(const auto& node : graph.nodes())
        if(node.group_id == group.id) members.push_back(&node);
    if(members.empty()) return;
    const Layout canvas_layout{.width = layout_extent(area.width), .height = layout_extent(area.height),
        .padding = 0, .positioned = true, .x = area.x, .y = area.y};
    for(std::size_t start = 0; start < members.size(); start += shapes_per_canvas) {
        Drawing drawing;
        const auto end = std::min(members.size(), start + shapes_per_canvas);
        for(std::size_t index = start; index < end; ++index) {
            const auto& b = members[index]->bounds;
            drawing.rectangle({b.x * scale, b.y * scale, b.width * scale, b.height * scale},
                tint(group.color), {group.color, outline_width}, corner_radius);
        }
        document.canvas(group_prefix(group.id) + "/shapes/" + std::to_string(start / shapes_per_canvas),
            std::move(drawing), canvas_layout);
    }
    // The column heading sits above the first node, matching Pf's diagram.
    const auto& first = members.front()->bounds;
    document.text(group_prefix(group.id) + "/heading", group.name,
        {.width = layout_extent(first.width * scale), .padding = 0, .positioned = true,
         .x = area.x + first.x * scale, .y = area.y + (first.y - heading_offset) * scale});
    for(const auto* node : members) {
        const auto& b = node->bounds;
        const float room = b.height * scale - label_offset * 2;
        document.text(group_prefix(group.id) + "/label/" + node->id, node->label,
            {.width = layout_extent(b.width * scale - node_padding * 2), .padding = 0, .positioned = true,
             .x = area.x + b.x * scale + node_padding, .y = area.y + b.y * scale + label_offset,
             .lines = static_cast<unsigned>(std::max(1.0f, room / assumed_line_height))});
    }
}

// 経路の終端に向きを示す折れ線を足す。最後の区間が長さを持たない経路には描かない。
void draw_arrow(Drawing& drawing, const std::vector<Point>& points, float scale) {
    const Point tip{points.back().x * scale, points.back().y * scale};
    const Point previous{points[points.size() - 2].x * scale, points[points.size() - 2].y * scale};
    const float dx = tip.x - previous.x, dy = tip.y - previous.y;
    const float length = std::sqrt(dx * dx + dy * dy);
    if(!(length > 0)) return;
    const float angle = std::atan2(dy, dx);
    const auto wing = [&](float offset) {
        return Point{tip.x - arrow_length * std::cos(angle + offset),
                     tip.y - arrow_length * std::sin(angle + offset)};
    };
    drawing.polyline({wing(-arrow_spread), tip, wing(arrow_spread)}, {edge_color, edge_width});
}

void declare_edges(Document& document, const Graph& graph, Rect area, float scale) {
    std::vector<const GraphEdge*> drawn;
    for(const auto& edge : graph.edges()) {
        const auto* from = find_node(graph, edge.from);
        const auto* to = find_node(graph, edge.to);
        // A relation is only meaningful while both of its ends are on screen.
        if(from && to && group_visible(graph, from->group_id) && group_visible(graph, to->group_id))
            drawn.push_back(&edge);
    }
    if(drawn.empty()) return;
    const Layout canvas_layout{.width = layout_extent(area.width), .height = layout_extent(area.height),
        .padding = 0, .positioned = true, .x = area.x, .y = area.y};
    for(std::size_t start = 0; start < drawn.size(); start += edges_per_canvas) {
        Drawing drawing;
        const auto end = std::min(drawn.size(), start + edges_per_canvas);
        for(std::size_t index = start; index < end; ++index) {
            std::vector<Point> points;
            points.reserve(drawn[index]->points.size());
            for(const auto& point : drawn[index]->points) points.push_back({point.x * scale, point.y * scale});
            const Stroke stroke{edge_color, edge_width,
                drawn[index]->dashed ? edge_dash : 0.f, drawn[index]->dashed ? edge_gap : 0.f};
            drawing.polyline(points, stroke);
            draw_arrow(drawing, drawn[index]->points, scale);
        }
        document.canvas("graph/edges/" + std::to_string(start / edges_per_canvas), std::move(drawing), canvas_layout);
    }
}
}

Rect graph_area(const GraphInfo& info, Rect available) {
    if(available.width <= 0 || available.height <= 0 || info.width <= 0 || info.height <= 0)
        return {available.x, available.y, 0, 0};
    const float scale = std::min(available.width / info.width, available.height / info.height);
    const float width = info.width * scale, height = info.height * scale;
    return {available.x + (available.width - width) / 2, available.y + (available.height - height) / 2, width, height};
}

Document graph_document(const Graph& graph, const Viewport& viewport,
    std::function<void(const std::string& group_id)> toggle) {
    Document document;
    // A hidden or degenerate host declares nothing, matching the overlay lifecycle contract.
    if(!viewport.visible || viewport.width <= 0 || viewport.height <= 0 || !(viewport.dpi_scale > 0)) return document;
    const Rect area = graph_area(graph.info(),
        {0, 0, viewport.width / viewport.dpi_scale, viewport.height / viewport.dpi_scale});
    if(area.width <= 0 || area.height <= 0) return document;
    const float scale = area.width / graph.info().width;
    // Relations first so the nodes stay readable where a route passes behind them.
    declare_edges(document, graph, area, scale);
    for(const auto& group : graph.groups())
        if(group.visible) declare_nodes(document, graph, group, area, scale);
    Layout controls;
    controls.width = controls_width;
    controls.positioned = true;
    controls.x = controls_margin;
    controls.y = controls_margin;
    // Declared last so the toggles stay above the drawn graph.
    document.panel("graph/controls", [&] {
        const auto& info = graph.info();
        document.text("graph/controls/title",
            (info.project.empty() ? std::string() : info.project + " ") + (info.title.empty() ? std::string("Graph") : info.title));
        for(const auto& group : graph.groups()) {
            const std::string id = group.id;
            const std::string name = group.name.empty() ? id : group.name;
            document.button("graph/toggle/" + id, (group.visible ? "ON  " : "OFF ") + name, [toggle, id] { toggle(id); });
        }
    }, controls);
    return document;
}
}
