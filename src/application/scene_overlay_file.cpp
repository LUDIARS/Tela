// @spec SPEC-TL-SCENE-OVERLAY
#include <tela/scene_overlay.hpp>
#include <fstream>
#include <iomanip>
#include <optional>
#include <sstream>
#include <stdexcept>

namespace tela {
namespace {
constexpr std::uintmax_t max_file_bytes = 4 * 1024 * 1024;

void finish(std::istringstream& row) {
    row >> std::ws;
    if(!row.eof()) throw std::invalid_argument("Trailing scene overlay data");
}
bool read_flag(std::istringstream& row) {
    int flag = -1;
    if(!(row >> flag) || (flag != 0 && flag != 1)) throw std::invalid_argument("Scene visibility must be 0 or 1");
    return flag == 1;
}
SceneOverlayFrame read_frame(std::istringstream& row) {
    SceneOverlayFrame frame;
    if(!(row >> std::quoted(frame.name) >> frame.width >> frame.height)) throw std::invalid_argument("Invalid scene overlay frame");
    finish(row);
    return frame;
}
SceneOverlayScene read_scene(std::istringstream& row) {
    SceneOverlayScene scene;
    if(!(row >> std::quoted(scene.id) >> std::quoted(scene.name))) throw std::invalid_argument("Invalid scene overlay scene");
    scene.visible = read_flag(row);
    finish(row);
    return scene;
}
SceneOverlayElement read_element(std::istringstream& row) {
    SceneOverlayElement element;
    auto& b = element.bounds;
    if(!(row >> std::quoted(element.scene_id) >> std::quoted(element.id) >> std::quoted(element.kind)
             >> std::quoted(element.label) >> b.x >> b.y >> b.width >> b.height))
        throw std::invalid_argument("Invalid scene overlay element");
    finish(row);
    return element;
}
}

SceneOverlay load_scene_overlay(const std::filesystem::path& path) {
    if(std::filesystem::file_size(path) > max_file_bytes) throw std::invalid_argument("Scene overlay file exceeds 4 MiB");
    std::ifstream input(path, std::ios::binary);
    std::string line;
    if(!input || !std::getline(input, line) || line != "TELA_SCENE_OVERLAY 1")
        throw std::invalid_argument("Unsupported scene overlay file");
    std::optional<SceneOverlayFrame> frame;
    std::vector<SceneOverlayScene> scenes;
    std::vector<SceneOverlayElement> elements;
    while(std::getline(input, line)) {
        if(line.empty()) continue;
        std::istringstream row(line);
        std::string record;
        row >> record;
        if(record == "frame") {
            if(frame) throw std::invalid_argument("Duplicate scene overlay frame");
            frame = read_frame(row);
        } else if(record == "scene") {
            scenes.push_back(read_scene(row));
        } else if(record == "element") {
            elements.push_back(read_element(row));
        } else {
            throw std::invalid_argument("Unknown scene overlay record");
        }
        // Stop reading an oversized file early; the model repeats the limits for in-memory callers.
        if(scenes.size() > scene_overlay_max_scenes || elements.size() > scene_overlay_max_elements)
            throw std::invalid_argument("Scene overlay limit exceeded");
    }
    if(input.bad()) throw std::runtime_error("Scene overlay read failed");
    if(!frame) throw std::invalid_argument("Scene overlay frame is required");
    return SceneOverlay(std::move(*frame), std::move(scenes), std::move(elements));
}
}
