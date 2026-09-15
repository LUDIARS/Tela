// @implements SPEC-TL-SPEC-VIEW
// @spec Spec view
#include <tela/spec_view.hpp>
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
// Same bytes as the Pf export contract test (Praeforma spec-view.test.ts).
// The group names are the UTF-8 labels Pf writes for the status axis.
const char* const exported =
    "TELA_SPEC_VIEW 1\n"
    "view \"Tela\" \"0.0.3\" 1136 332\n"
    "group \"draft\" \"\xe4\xb8\x8b\xe6\x9b\xb8\xe3\x81\x8d\" 1\n"
    "group \"approved\" \"\xe7\xa2\xba\xe5\xae\x9a\" 1\n"
    "card \"draft\" \"TL-A\" \"Overlay \\\"core\\\"\" \"draft\" 2 24 56 260 96\n"
    "card \"approved\" \"TL-B\" \"Bridge\\\\IPC\" \"approved\" 1 24 212 260 96\n";

const tela::Element* find(const tela::Document& document, const std::string& id) {
    const auto& elements = document.elements();
    const auto found = std::find_if(elements.begin(), elements.end(), [&](const auto& element) { return element.id == id; });
    return found == elements.end() ? nullptr : &*found;
}

void loading(const std::filesystem::path& path) {
    write(path, exported);
    const auto view = tela::load_spec_view(path);
    require(view.info() == tela::SpecViewInfo{"Tela", "0.0.3", 1136, 332}, "view header is read");
    require(view.groups().size() == 2, "groups are read in order");
    require(view.groups()[0].id == "draft" && view.groups()[1].id == "approved", "group order is preserved");
    require(view.groups()[0].visible && view.groups()[1].visible, "initial visibility is read");
    require(view.cards().size() == 2, "cards are read");
    require(view.cards()[0].title == "Overlay \"core\"" && view.cards()[1].title == "Bridge\\IPC", "quoted titles are unescaped");
    require(view.cards()[0].version == 2 && view.cards()[1].status == "approved", "card version and status are read");
    require(view.cards()[1].bounds == tela::Rect{24, 212, 260, 96}, "card bounds are read");
    for(const char* broken : {
        "TELA_SPEC_VIEW 9\nview \"P\" \"1\" 10 10\ngroup \"a\" \"A\" 1\n",
        "TELA_SPEC_VIEW 1\ngroup \"a\" \"A\" 1\n",
        "TELA_SPEC_VIEW 1\nview \"P\" \"1\" 10 10\n",
        "TELA_SPEC_VIEW 1\nview \"P\" \"1\" 10 10\nview \"Q\" \"1\" 10 10\ngroup \"a\" \"A\" 1\n",
        "TELA_SPEC_VIEW 1\nview \"P\" \"1\" 10 10\ngroup \"a\" \"A\" 2\n",
        "TELA_SPEC_VIEW 1\nview \"P\" \"1\" 10 10\ngroup \"a\" \"A\" 1 extra\n",
        "TELA_SPEC_VIEW 1\nview \"P\" \"1\" 10 10\ngroup \"a\" \"A\" 1\ngroup \"a\" \"B\" 1\n",
        "TELA_SPEC_VIEW 1\nview \"P\" \"1\" 10 10\ngroup \"a\" \"A\" 1\ncard \"x\" \"C\" \"T\" \"draft\" 1 0 0 1 1\n",
        "TELA_SPEC_VIEW 1\nview \"P\" \"1\" 10 10\ngroup \"a\" \"A\" 1\ncard \"a\" \"C\" \"T\" \"draft\" 1 0 0 0 1\n",
        "TELA_SPEC_VIEW 1\nview \"P\" \"1\" 10 10\ngroup \"a\" \"A\" 1\ncard \"a\" \"C\" \"T\" \"draft\" -1 0 0 1 1\n",
        "TELA_SPEC_VIEW 1\nview \"P\" \"1\" 10 10\ngroup \"a\" \"A\" 1\ncard \"a\" \"C\" \"T\" \"draft\" 1 0 0 1 1\ncard \"a\" \"C\" \"U\" \"draft\" 1 0 0 1 1\n",
        "TELA_SPEC_VIEW 1\nview \"P\" \"1\" 0 10\ngroup \"a\" \"A\" 1\n",
        "TELA_SPEC_VIEW 1\nview \"P\" \"1\" 10 10\nwidget \"a\"\n",
    }) {
        write(path, broken);
        try { tela::load_spec_view(path); throw std::logic_error(broken); }
        catch(const std::invalid_argument&) {}
    }
}

void composition(const std::filesystem::path& path) {
    write(path, exported);
    auto view = tela::load_spec_view(path);
    require(tela::spec_view_area({"P", "1", 1000, 500}, {0, 0, 500, 500}) == tela::Rect{0, 125, 500, 250},
        "view fits the viewport keeping its aspect ratio");
    const tela::Viewport viewport{"host", "view", 1, 0, 0, 1136, 332, 1, true, true}; // 1:1 logical
    std::string toggled;
    const auto document = tela::spec_view_document(view, viewport, [&](const std::string& id) { toggled = id; });
    const auto* code = find(document, "spec-view/group/draft/code/TL-A");
    require(code != nullptr && code->label == "TL-A draft v2", "cards declare their code, status and version");
    require(code->layout.x == 30 && code->layout.y == 60, "card text uses fitted logical coordinates");
    const auto* title = find(document, "spec-view/group/draft/title/TL-A");
    require(title != nullptr && title->label == "Overlay \"core\"" && title->layout.y == 82, "cards declare their title below the code");
    require(find(document, "spec-view/group/draft/shapes/0") != nullptr, "cards are drawn as one chunked canvas");
    const auto* toggle = find(document, "spec-view/toggle/approved");
    require(toggle != nullptr && toggle->input == tela::InputPolicy::exclusive, "every group keeps an actionable toggle");
    toggle->action();
    require(toggled == "approved", "toggle reports its group");

    view.set_visible("approved", false);
    const auto hidden_group = tela::spec_view_document(view, viewport, {});
    require(find(hidden_group, "spec-view/group/approved/code/TL-B") == nullptr, "hidden group declares no cards");
    require(find(hidden_group, "spec-view/toggle/approved") != nullptr, "hidden group keeps its toggle");
    tela::Runtime runtime;
    runtime.viewport(viewport);
    runtime.document(hidden_group);
    runtime.frame_presented();
    runtime.document(tela::spec_view_document(view, viewport, {}));
    require(!runtime.needs_frame(), "identical spec view must not schedule a frame");

    auto hidden = viewport;
    hidden.visible = false;
    require(tela::spec_view_document(view, hidden, {}).elements().empty(), "hidden host declares nothing");
    try { view.set_visible("missing", true); throw std::logic_error("unknown group accepted"); }
    catch(const std::invalid_argument&) {}
}

void limits() {
    std::vector<tela::SpecViewCard> many;
    for(std::size_t index = 0; index <= tela::spec_view_max_cards; ++index)
        many.push_back({"a", "C" + std::to_string(index), "T", "draft", 1, {0, 0, 1, 1}});
    try {
        const tela::SpecView rejected({"P", "1", 10, 10}, {{"a", "A", true}}, many);
        throw std::logic_error("card limit accepted");
    } catch(const std::invalid_argument&) {}
    try {
        const tela::SpecView rejected({"P", "1", 10, 10}, {}, {});
        throw std::logic_error("empty group list accepted");
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
