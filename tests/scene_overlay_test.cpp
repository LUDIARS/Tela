// @implements SPEC-TL-SCENE-OVERLAY
// @spec Scene overlay
#include <tela/scene_overlay.hpp>
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
// Same bytes as the Pf export contract test (Praeforma scene-layers.test.ts).
const char* const exported =
    "TELA_SCENE_OVERLAY 1\n"
    "frame \"Battle\" 1000 500\n"
    "scene \"battle\" \"Battle \\\"main\\\"\" 1\n"
    "scene \"layer-1\" \"Option\\\\Menu PC\" 0\n"
    "element \"battle\" \"attack\" \"button\" \"attack\" 10 20 100 40\n"
    "element \"layer-1\" \"close\" \"button\" \"close\" 100 10 150 20\n";

const tela::Element* find(const tela::Document& document, const std::string& id) {
    const auto& elements = document.elements();
    const auto found = std::find_if(elements.begin(), elements.end(), [&](const auto& element) { return element.id == id; });
    return found == elements.end() ? nullptr : &*found;
}

void loading(const std::filesystem::path& path) {
    write(path, exported);
    const auto overlay = tela::load_scene_overlay(path);
    require(overlay.frame() == tela::SceneOverlayFrame{"Battle", 1000, 500}, "frame is read");
    require(overlay.scenes().size() == 2, "scenes are read in order");
    require(overlay.scenes()[0].name == "Battle \"main\"" && overlay.scenes()[1].name == "Option\\Menu PC", "quoted names are unescaped");
    require(overlay.scenes()[0].visible && !overlay.scenes()[1].visible, "initial visibility is read");
    require(overlay.elements().size() == 2 && overlay.elements()[1].bounds == tela::Rect{100, 10, 150, 20}, "element bounds are read");
    for(const char* broken : {
        "TELA_SCENE_OVERLAY 9\nframe \"F\" 10 10\nscene \"a\" \"A\" 1\n",
        "TELA_SCENE_OVERLAY 1\nscene \"a\" \"A\" 1\n",
        "TELA_SCENE_OVERLAY 1\nframe \"F\" 10 10\n",
        "TELA_SCENE_OVERLAY 1\nframe \"F\" 10 10\nframe \"G\" 10 10\nscene \"a\" \"A\" 1\n",
        "TELA_SCENE_OVERLAY 1\nframe \"F\" 10 10\nscene \"a\" \"A\" 2\n",
        "TELA_SCENE_OVERLAY 1\nframe \"F\" 10 10\nscene \"a\" \"A\" 1 extra\n",
        "TELA_SCENE_OVERLAY 1\nframe \"F\" 10 10\nscene \"a\" \"A\" 1\nscene \"a\" \"B\" 1\n",
        "TELA_SCENE_OVERLAY 1\nframe \"F\" 10 10\nscene \"a\" \"A\" 1\nelement \"x\" \"e\" \"box\" \"E\" 0 0 1 1\n",
        "TELA_SCENE_OVERLAY 1\nframe \"F\" 10 10\nscene \"a\" \"A\" 1\nelement \"a\" \"e\" \"box\" \"E\" 0 0 0 1\n",
        "TELA_SCENE_OVERLAY 1\nframe \"F\" 10 10\nscene \"a\" \"A\" 1\nwidget \"a\"\n",
    }) {
        write(path, broken);
        try { tela::load_scene_overlay(path); throw std::logic_error(broken); }
        catch(const std::invalid_argument&) {}
    }
}

void composition(const std::filesystem::path& path) {
    write(path, exported);
    auto overlay = tela::load_scene_overlay(path);
    require(tela::scene_overlay_area(overlay.frame(), {0, 0, 500, 500}) == tela::Rect{0, 125, 500, 250},
        "frame fits the viewport keeping its aspect ratio");
    const tela::Viewport view{"host", "view", 1, 0, 0, 1000, 1000, 2, true, true}; // 500x500 logical
    std::string toggled;
    const auto document = tela::scene_overlay_document(overlay, view, [&](const std::string& id) { toggled = id; });
    const auto* label = find(document, "scene-overlay/scene/battle/label/attack");
    require(label != nullptr, "visible scene declares its elements");
    require(label->layout.x == 5 && label->layout.y == 135, "labels use fitted logical coordinates");
    require(find(document, "scene-overlay/scene/layer-1/label/close") == nullptr, "hidden scene declares no elements");
    const auto* toggle = find(document, "scene-overlay/toggle/layer-1");
    require(toggle != nullptr && toggle->input == tela::InputPolicy::exclusive, "hidden scene keeps an actionable toggle");
    toggle->action();
    require(toggled == "layer-1", "toggle reports its scene");

    overlay.set_visible("layer-1", true);
    const auto shown = tela::scene_overlay_document(overlay, view, {});
    require(find(shown, "scene-overlay/scene/layer-1/label/close") != nullptr, "toggled scene becomes visible");
    tela::Runtime runtime;
    runtime.viewport(view);
    runtime.document(shown);
    runtime.frame_presented();
    runtime.document(tela::scene_overlay_document(overlay, view, {}));
    require(!runtime.needs_frame(), "identical scene overlay must not schedule a frame");

    auto hidden = view;
    hidden.visible = false;
    require(tela::scene_overlay_document(overlay, hidden, {}).elements().empty(), "hidden host declares nothing");
    try { overlay.set_visible("missing", true); throw std::logic_error("unknown scene accepted"); }
    catch(const std::invalid_argument&) {}
}

void limits() {
    const std::vector<tela::SceneOverlayElement> many(tela::scene_overlay_max_elements + 1,
        tela::SceneOverlayElement{"a", "e", "box", "E", {0, 0, 1, 1}});
    try {
        const tela::SceneOverlay rejected({"F", 10, 10}, {{"a", "A", true}}, many);
        throw std::logic_error("element limit accepted");
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
