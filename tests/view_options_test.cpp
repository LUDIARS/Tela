// @implements SPEC-TL-VIEW-HOST
// @spec View host
#include "../examples/windows/view_options.hpp"
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {
void require(bool value, const char* why) { if(!value) throw std::runtime_error(why); }

ViewOptions parse(std::vector<const char*> args) {
    std::vector<char*> argv{const_cast<char*>("tela_view")};
    for(auto* arg : args) argv.push_back(const_cast<char*>(arg));
    return viewOptions(static_cast<int>(argv.size()), argv.data());
}
void rejects(std::vector<const char*> args, const char* why) {
    try { parse(std::move(args)); throw std::logic_error(why); }
    catch(const std::invalid_argument&) {}
}
}

int main() {
    try {
        const auto shown = parse({"--font", "f.ttf", "--spec-view", "v.tela"});
        require(shown.font == "f.ttf" && shown.specView == "v.tela", "the font and content are read");
        require(shown.title == "Tela" && !shown.fullscreen, "a view starts windowed with a default title");
        require(shown.width == 0 && shown.height == 0, "no size means the content's exported size");

        const auto sized = parse({"--font", "f.ttf", "--spec-view", "v.tela", "--fullscreen",
                                  "--width", "1280", "--height", "720", "--font-size", "20",
                                  "--seconds", "30", "--title", "Pf"});
        require(sized.fullscreen && sized.width == 1280 && sized.height == 720, "size and fullscreen are read");
        require(sized.fontSize == 20 && sized.seconds == 30 && sized.title == "Pf", "text size, duration and title are read");

        // A run without a font or content cannot draw anything, so it fails instead of showing an empty window.
        rejects({"--spec-view", "v.tela"}, "missing font accepted");
        rejects({"--font", "f.ttf"}, "missing content accepted");
        rejects({"--font", "f.ttf", "--spec-view", "v.tela", "--font-size", "4"}, "tiny font size accepted");
        rejects({"--font", "f.ttf", "--spec-view", "v.tela", "--font-size", "200"}, "huge font size accepted");
        rejects({"--font", "f.ttf", "--spec-view", "v.tela", "--width", "10"}, "tiny width accepted");
        rejects({"--font", "f.ttf", "--spec-view", "v.tela", "--seconds", "0"}, "zero duration accepted");
        rejects({"--font", "f.ttf", "--spec-view", "v.tela", "--sideways", "1"}, "unknown option accepted");
        rejects({"--font"}, "missing option value accepted");
        return 0;
    } catch(const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 2;
    }
}
