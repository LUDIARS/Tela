// @implements SPEC-TL-GRAPH
// @spec Graph view
#include <tela/graph.hpp>
#include <tela/runtime.hpp>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace {
void require(bool value, const char* why) { if(!value) throw std::runtime_error(why); }
void write(const std::filesystem::path& path, const std::string& text) {
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    output << text;
    if(!output) throw std::runtime_error("cannot write fixture");
}
// Same bytes as the Pf export contract test (Praeforma domain-graph.test.ts).
const char* const exported =
    "TELA_GRAPH 1\n"
    "graph \"Praeforma\" \"\xe3\x83\x89\xe3\x83\xa1\xe3\x82\xa4\xe3\x83\xb3\xe9\x96\xa2\xe4\xbf\x82\xe5\x9b\xb3\" 1040 210\n"
    "group \"core\" \"\xe3\x82\xb3\xe3\x82\xa2\xe3\x83\x89\xe3\x83\xa1\xe3\x82\xa4\xe3\x83\xb3\" 1 128 84 189\n"
    "group \"business\" \"\xe3\x83\x93\xe3\x82\xb8\xe3\x83\x8d\xe3\x82\xb9\xe3\x83\x89\xe3\x83\xa1\xe3\x82\xa4\xe3\x83\xb3\" 1 40 111 168\n"
    "group \"unclassified\" \"\xe6\x9c\xaa\xe5\x88\x86\xe9\xa1\x9e\" 1 103 113 126\n"
    "node \"core\" \"core-1\" \"\xe3\x82\xb3\xe3\x82\xa2 \\\"A\\\"\" 40 60 220 80\n"
    "node \"business\" \"biz-1\" \"\xe3\x83\x93\xe3\x82\xb8\xe3\x83\x8d\xe3\x82\xb9\\\\B\" 380 60 220 80\n"
    "node \"unclassified\" \"loose\" \"\xe6\x9c\xaa\xe5\x88\x86\xe9\xa1\x9e" "C\" 720 60 220 80\n"
    "edge \"core-1:biz-1\" \"core-1\" \"biz-1\" 0\n"
    "point \"core-1:biz-1\" 260 100\n"
    "point \"core-1:biz-1\" 270.58 100\n"
    "point \"core-1:biz-1\" 279.92 100\n"
    "point \"core-1:biz-1\" 288.21 100\n"
    "point \"core-1:biz-1\" 295.63 100\n"
    "point \"core-1:biz-1\" 302.33 100\n"
    "point \"core-1:biz-1\" 308.52 100\n"
    "point \"core-1:biz-1\" 314.35 100\n"
    "point \"core-1:biz-1\" 320 100\n"
    "point \"core-1:biz-1\" 325.65 100\n"
    "point \"core-1:biz-1\" 331.48 100\n"
    "point \"core-1:biz-1\" 337.67 100\n"
    "point \"core-1:biz-1\" 344.38 100\n"
    "point \"core-1:biz-1\" 351.79 100\n"
    "point \"core-1:biz-1\" 360.08 100\n"
    "point \"core-1:biz-1\" 369.42 100\n"
    "point \"core-1:biz-1\" 380 100\n";

const char* const minimal =
    "TELA_GRAPH 1\n"
    "graph \"P\" \"T\" 100 100\n"
    "group \"g\" \"G\" 1 10 20 30\n"
    "node \"g\" \"n\" \"N\" 0 0 10 10\n";

const tela::Element* find(const tela::Document& document, const std::string& id) {
    const auto& elements = document.elements();
    const auto found = std::find_if(elements.begin(), elements.end(), [&](const auto& element) { return element.id == id; });
    return found == elements.end() ? nullptr : &*found;
}

void loading(const std::filesystem::path& path) {
    write(path, exported);
    const auto graph = tela::load_graph(path);
    require(graph.info() == tela::GraphInfo{"Praeforma", "\xe3\x83\x89\xe3\x83\xa1\xe3\x82\xa4\xe3\x83\xb3\xe9\x96\xa2\xe4\xbf\x82\xe5\x9b\xb3", 1040, 210},
        "the header is read");
    require(graph.groups().size() == 3, "every classification column is read");
    require(graph.groups()[0].color == tela::Color{128, 84, 189, 255}, "group colours are read");
    require(graph.groups()[0].visible && graph.groups()[2].id == "unclassified", "visibility and order are read");
    require(graph.nodes().size() == 3, "every node is read");
    require(graph.nodes()[0].label == "\xe3\x82\xb3\xe3\x82\xa2 \"A\"", "quoted labels are unescaped");
    require(graph.nodes()[1].label == "\xe3\x83\x93\xe3\x82\xb8\xe3\x83\x8d\xe3\x82\xb9\\B", "escaped backslashes are unescaped");
    require(graph.nodes()[0].bounds == tela::Rect{40, 60, 220, 80}, "node bounds are read");
    require(graph.edges().size() == 1 && !graph.edges()[0].dashed, "the membership relation is solid");
    require(graph.edges()[0].points.size() == 17, "the routed polyline is read whole");
    require(graph.edges()[0].points.front() == tela::Point{260, 100}, "the route starts at the source node");
    require(graph.edges()[0].points.back() == tela::Point{380, 100}, "the route ends at the target node");
    for(const char* broken : {
        "TELA_GRAPH 9\ngraph \"P\" \"T\" 10 10\ngroup \"g\" \"G\" 1 0 0 0\nnode \"g\" \"n\" \"N\" 0 0 1 1\n",
        "TELA_GRAPH 1\ngroup \"g\" \"G\" 1 0 0 0\nnode \"g\" \"n\" \"N\" 0 0 1 1\n",
        "TELA_GRAPH 1\ngraph \"P\" \"T\" 10 10\n",
        "TELA_GRAPH 1\ngraph \"P\" \"T\" 10 10\ngraph \"Q\" \"T\" 10 10\ngroup \"g\" \"G\" 1 0 0 0\nnode \"g\" \"n\" \"N\" 0 0 1 1\n",
        "TELA_GRAPH 1\ngraph \"P\" \"T\" 10 10\ngroup \"g\" \"G\" 2 0 0 0\nnode \"g\" \"n\" \"N\" 0 0 1 1\n",
        "TELA_GRAPH 1\ngraph \"P\" \"T\" 10 10\ngroup \"g\" \"G\" 1 0 0 300\nnode \"g\" \"n\" \"N\" 0 0 1 1\n",
        "TELA_GRAPH 1\ngraph \"P\" \"T\" 10 10\ngroup \"g\" \"G\" 1 0 0 0\ngroup \"g\" \"H\" 1 0 0 0\nnode \"g\" \"n\" \"N\" 0 0 1 1\n",
        "TELA_GRAPH 1\ngraph \"P\" \"T\" 10 10\ngroup \"g\" \"G\" 1 0 0 0\nnode \"x\" \"n\" \"N\" 0 0 1 1\n",
        "TELA_GRAPH 1\ngraph \"P\" \"T\" 10 10\ngroup \"g\" \"G\" 1 0 0 0\nnode \"g\" \"n\" \"N\" 0 0 0 1\n",
        "TELA_GRAPH 1\ngraph \"P\" \"T\" 10 10\ngroup \"g\" \"G\" 1 0 0 0\nnode \"g\" \"n\" \"N\" 0 0 1 1\nedge \"e\" \"n\" \"missing\" 0\npoint \"e\" 0 0\npoint \"e\" 1 1\n",
        "TELA_GRAPH 1\ngraph \"P\" \"T\" 10 10\ngroup \"g\" \"G\" 1 0 0 0\nnode \"g\" \"n\" \"N\" 0 0 1 1\nedge \"e\" \"n\" \"n\" 0\npoint \"e\" 0 0\n",
        "TELA_GRAPH 1\ngraph \"P\" \"T\" 10 10\ngroup \"g\" \"G\" 1 0 0 0\nnode \"g\" \"n\" \"N\" 0 0 1 1\npoint \"e\" 0 0\n",
        "TELA_GRAPH 1\ngraph \"P\" \"T\" 10 10\ngroup \"g\" \"G\" 1 0 0 0\nnode \"g\" \"n\" \"N\" 0 0 1 1 extra\n",
        "TELA_GRAPH 1\ngraph \"P\" \"T\" 0 10\ngroup \"g\" \"G\" 1 0 0 0\nnode \"g\" \"n\" \"N\" 0 0 1 1\n",
        "TELA_GRAPH 1\ngraph \"P\" \"T\" 10 10\ngroup \"g\" \"G\" 1 0 0 0\nwidget \"g\"\n",
    }) {
        write(path, broken);
        try { tela::load_graph(path); throw std::logic_error(broken); }
        catch(const std::invalid_argument&) {}
    }
    write(path, minimal);
    require(tela::load_graph(path).edges().empty(), "a graph without relations still loads");
}

void composition(const std::filesystem::path& path) {
    write(path, exported);
    auto graph = tela::load_graph(path);
    require(tela::graph_area({"P", "T", 1000, 500}, {0, 0, 500, 500}) == tela::Rect{0, 125, 500, 250},
        "the graph fits the viewport keeping its aspect ratio");
    const tela::Viewport viewport{"host", "view", 1, 0, 0, 1040, 210, 1, true, true}; // 1:1 logical
    std::string toggled;
    const auto document = tela::graph_document(graph, viewport, [&](const std::string& id) { toggled = id; });
    const auto* label = find(document, "graph/group/core/label/core-1");
    require(label != nullptr && label->layout.x == 46 && label->layout.y == 66, "nodes label themselves inside their box");
    require(label->layout.lines == 2, "a node label takes the rows its box allows");
    require(find(document, "graph/group/core/heading") != nullptr, "each visible column is headed");
    require(find(document, "graph/group/core/shapes/0") != nullptr, "nodes are drawn as one chunked canvas");
    const auto* edges = find(document, "graph/edges/0");
    require(edges != nullptr && edges->drawing.shapes().size() == 1, "visible relations are drawn together");
    require(edges->drawing.shapes()[0].points.size() == 17, "the exported route is drawn as given, not re-curved");
    require(edges->drawing.shapes()[0].stroke.dash == 0, "a membership relation is solid");
    const auto* toggle = find(document, "graph/toggle/business");
    require(toggle != nullptr && toggle->input == tela::InputPolicy::exclusive, "every column keeps an actionable toggle");
    toggle->action();
    require(toggled == "business", "the toggle reports its column");

    graph.set_visible("business", false);
    const auto hidden_group = tela::graph_document(graph, viewport, {});
    require(find(hidden_group, "graph/group/business/label/biz-1") == nullptr, "a hidden column declares no nodes");
    require(find(hidden_group, "graph/edges/0") == nullptr, "a relation into a hidden column is not drawn");
    require(find(hidden_group, "graph/toggle/business") != nullptr, "a hidden column keeps its toggle");
    tela::Runtime runtime;
    runtime.viewport(viewport);
    runtime.document(hidden_group);
    runtime.frame_presented();
    runtime.document(tela::graph_document(graph, viewport, {}));
    require(!runtime.needs_frame(), "an identical graph must not schedule a frame");

    auto hidden = viewport;
    hidden.visible = false;
    require(tela::graph_document(graph, hidden, {}).elements().empty(), "a hidden host declares nothing");
    try { graph.set_visible("missing", true); throw std::logic_error("unknown column accepted"); }
    catch(const std::invalid_argument&) {}
}

void limits() {
    std::vector<tela::GraphNode> many;
    for(std::size_t index = 0; index <= tela::graph_max_nodes; ++index)
        many.push_back({"g", "n" + std::to_string(index), "N", {0, 0, 1, 1}});
    try {
        const tela::Graph rejected({"P", "T", 10, 10}, {{"g", "G", true, {1, 2, 3, 255}}}, many, {});
        throw std::logic_error("node limit accepted");
    } catch(const std::invalid_argument&) {}
    try {
        const tela::Graph rejected({"P", "T", 10, 10}, {}, {}, {});
        throw std::logic_error("empty graph accepted");
    } catch(const std::invalid_argument&) {}
}
}

int main(int argc, char** argv) {
    if(argc != 2) return 1;
    const std::filesystem::path path = argv[1];
    try {
        loading(path);
        composition(path);
        limits();
        std::filesystem::remove(path);
        return 0;
    } catch(const std::exception& e) {
        std::cerr << e.what() << '\n';
        std::error_code ignored;
        std::filesystem::remove(path, ignored);
        return 2;
    }
}
