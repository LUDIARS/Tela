// @implements SPEC-TL-SPEC-VIEW
// @spec Spec view
#include <tela/spec_view.hpp>
#include <tela/runtime.hpp>
#include <algorithm>
#include <cmath>
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
    "view \"Tela\" \"0.0.3\" 1220 332\n"
    "group \"draft\" \"\xe4\xb8\x8b\xe6\x9b\xb8\xe3\x81\x8d\" 1\n"
    "group \"approved\" \"\xe7\xa2\xba\xe5\xae\x9a\" 1\n"
    "card \"draft\" \"TL-A\" \"Overlay \\\"core\\\"\" \"draft\" 2 24 56 380 96\n"
    "card \"approved\" \"TL-B\" \"Bridge\\\\IPC\" \"approved\" 1 24 212 380 96\n";

const tela::Element* find(const tela::Document& document, const std::string& id) {
    const auto& elements = document.elements();
    const auto found = std::find_if(elements.begin(), elements.end(), [&](const auto& element) { return element.id == id; });
    return found == elements.end() ? nullptr : &*found;
}

void loading(const std::filesystem::path& path) {
    write(path, exported);
    const auto view = tela::load_spec_view(path);
    require(view.info() == tela::SpecViewInfo{"Tela", "0.0.3", 1220, 332}, "view header is read");
    require(view.groups().size() == 2, "groups are read in order");
    require(view.groups()[0].id == "draft" && view.groups()[1].id == "approved", "group order is preserved");
    require(view.groups()[0].visible && view.groups()[1].visible, "initial visibility is read");
    require(view.cards().size() == 2, "cards are read");
    require(view.cards()[0].title == "Overlay \"core\"" && view.cards()[1].title == "Bridge\\IPC", "quoted titles are unescaped");
    require(view.cards()[0].version == 2 && view.cards()[1].status == "approved", "card version and status are read");
    require(view.cards()[1].bounds == tela::Rect{24, 212, 380, 96}, "card bounds are read");
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
    const tela::Viewport viewport{"host", "view", 1, 0, 0, 1500, 332, 1, true, true}; // picture 1:1 plus controls
    std::string toggled;
    const auto document = tela::spec_view_document(view, viewport, [&](const std::string& id) { toggled = id; });
    const auto* code = find(document, "spec-view/group/draft/code/TL-A");
    require(code != nullptr && code->label == "TL-A draft v2", "cards declare their code, status and version");
    require(code->layout.x == 310 && code->layout.y == 60, "card text uses fitted coordinates outside controls");
    const auto* title = find(document, "spec-view/group/draft/title/TL-A");
    require(title != nullptr && title->label == "Overlay \"core\"" && title->layout.y == 84, "title starts after a complete code line");
    require(title->layout.lines == 2 && title->layout.height == 48, "title wraps within the card's remaining full lines");
    const auto* shapes = find(document, "spec-view/group/draft/shapes/0");
    require(shapes != nullptr && shapes->drawing.shapes().size() == 1, "cards are drawn as one chunked canvas");
    require(shapes->drawing.shapes()[0].fill == tela::Color{44, 63, 82, 235},
        "cards pre-blend Pf's dark canvas with the group color so the text reads on any host");
    require(shapes->drawing.shapes()[0].stroke.color == tela::Color{120, 200, 255, 255}, "the outline keeps the group color");
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

void readable_views() {
    for(const unsigned count : {1u, 32u}) {
        std::vector<tela::SpecViewGroup> groups;
        for(unsigned i = 0; i < count; ++i) groups.push_back({std::to_string(i), "Group", true});
        tela::SpecView view({"P", "1", 1000, 500}, groups,
            {{"0", "A", "A long specification title", "draft", 1, {0, 0, 380, 120}}});
        for(const int width : {500, 1280, 3280}) {
            tela::Runtime runtime;
            runtime.viewport({"host", "view", 1, 0, 0, width, 1500, 1, true, true});
            const auto document = tela::spec_view_document(view, runtime.viewport(), {});
            const auto* code = find(document, "spec-view/group/0/code/A");
            const auto* title = find(document, "spec-view/group/0/title/A");
            require(code && title, "card text remains declared at all zoom levels");
            const float scale = (width - std::min(280.f, width * .35f)) / 1000;
            require(std::abs(code->layout.text_scale - scale) < .0001f, "font follows picture zoom");
            require(std::abs(title->layout.height - 72 * scale) < .001f, "three title rows follow zoom");
            runtime.document(document);
            const auto& placed = runtime.elements();
            const auto panel = std::find_if(placed.begin(), placed.end(), [](const auto& p) {return p.element.id == "spec-view/controls";});
            for(const auto& p : placed) {
                if(p.element.id.starts_with("spec-view/group/"))
                    require(tela::intersect(panel->bounds, p.bounds).width == 0, "controls never cover the picture");
                if(p.element.kind == tela::ElementKind::button)
                    require(p.clip.width > 0 && std::abs(p.clip.height - p.bounds.height) < .001f,
                        "all 32 controls remain inside the host");
            }
            runtime.frame_presented();
            runtime.document(tela::spec_view_document(view, runtime.viewport(), {}));
            require(!runtime.needs_frame(), "fitted view remains idle when unchanged");
        }
    }
}
}

// UX-TL-W9: a view taller than the host keeps scale 1 and is read page by page, whole cards only.
void paging() {
    std::vector<tela::SpecViewCard> cards;
    for(int i = 0; i < 9; ++i)
        cards.push_back({"a", "C" + std::to_string(i), "Title", "draft", 1, {10, 10 + 110.f * i, 380, 96}});
    tela::SpecView view({"P", "1", 400, 1000}, {{"a", "A", true}}, cards);
    const tela::Viewport viewport{"host", "view", 1, 0, 0, 1000, 400, 1, true, true};
    require(tela::spec_view_area(view.info(), {280, 0, 720, 400}) == tela::Rect{440, 0, 400, 1000},
        "a tall view keeps scale 1 from the top instead of shrinking its text");
    require(tela::spec_view_page(view.info(), viewport) == 400, "a page is the viewport height at scale 1");
    tela::SpecViewAction pressed{};
    bool navigated = false;
    const auto declare = [&] {
        return tela::spec_view_document(view, viewport, {}, {}, [&](tela::SpecViewAction a) { pressed = a; navigated = true; });
    };
    const auto position = [&](const tela::Document& document) {
        const auto* text = find(document, "spec-view/page/position");
        return text ? text->label : std::string();
    };
    auto document = declare();
    require(position(document) == "Cards 1-3 / 9", "the first page shows only the cards that fit whole");
    require(find(document, "spec-view/group/a/code/C3") == nullptr, "a card cut by the bottom edge is not declared");
    const auto* next = find(document, "spec-view/page/1");
    require(next && next->input == tela::InputPolicy::exclusive && next->label == "Next page", "page buttons take their own input");
    next->action();
    require(navigated && pressed == tela::SpecViewAction::next, "page buttons report their action");

    view.navigate(tela::SpecViewAction::next, 400);
    require(view.scroll() == 340, "the next page starts at the first card that was not shown whole");
    document = declare();
    require(position(document) == "Cards 4-6 / 9", "the position follows the shown cards");
    const auto* code = find(document, "spec-view/group/a/code/C3");
    require(code && code->layout.y == 4 && code->layout.text_scale == 1, "shown cards move up by the scroll at scale 1");
    require(find(document, "spec-view/group/a/code/C0") == nullptr, "cards above the page are not declared");
    const auto* shapes = find(document, "spec-view/group/a/shapes/0");
    require(shapes && shapes->layout.y == 0 && shapes->layout.height == 400 && shapes->drawing.shapes().size() == 3
        && shapes->drawing.shapes()[0].bounds.y == 0, "the card canvas covers the shown page only");

    view.navigate(tela::SpecViewAction::next, 400);
    require(view.scroll() == 670, "paging continues by whole cards");
    view.navigate(tela::SpecViewAction::next, 400);
    require(view.scroll() == 670 && position(declare()) == "Cards 7-9 / 9", "the last page stops at the end");
    view.navigate(tela::SpecViewAction::previous, 400);
    require(view.scroll() == 340, "going back shows the cards right above the page");
    view.navigate(tela::SpecViewAction::previous, 400);
    view.navigate(tela::SpecViewAction::previous, 400);
    require(view.scroll() == 0, "going back stops at the top");
    view.navigate(tela::SpecViewAction::next, 400);
    view.navigate(tela::SpecViewAction::first, 400);
    require(view.scroll() == 0, "first returns to the top");

    view.navigate(tela::SpecViewAction::next, 400);
    view.set_visible("a", false);
    view.set_visible("a", true);
    require(view.scroll() == 340, "group toggles keep the page");
    auto tall = viewport;
    tall.height = 2000;
    const auto fitted = tela::spec_view_document(view, tall, {});
    require(find(fitted, "spec-view/page/1") == nullptr && find(fitted, "spec-view/group/a/code/C0") != nullptr,
        "a host that fits the whole view shows every card without page controls");

    tela::Runtime runtime;
    runtime.viewport(viewport);
    runtime.document(declare());
    runtime.frame_presented();
    runtime.document(declare());
    require(!runtime.needs_frame(), "an unchanged page remains idle");
}

int main(int argc, char** argv) {
    if(argc != 2) return 1;
    const std::filesystem::path path = argv[1];
    try {
        loading(path);
        composition(path);
        limits();
        readable_views();
        paging();
        std::filesystem::remove(path);
        return 0;
    } catch(const std::exception& e) {
        std::cerr << e.what() << '\n';
        std::error_code ignored;
        std::filesystem::remove(path, ignored);
        return 2;
    }
}
