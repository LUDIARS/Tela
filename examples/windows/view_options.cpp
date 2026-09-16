#include "view_options.hpp"
#include <stdexcept>
#include <string_view>
// @spec View host
namespace {
int bounded(const std::string& value, int low, int high, const char* why) {
    const int parsed = std::stoi(value);
    if(parsed < low || parsed > high) throw std::invalid_argument(why);
    return parsed;
}
}
ViewOptions viewOptions(int argc, char** argv) {
    ViewOptions result;
    for(int i = 1; i < argc; ++i) {
        std::string_view arg = argv[i];
        if(arg == "--fullscreen") { result.fullscreen = true; continue; }
        if(i + 1 >= argc) throw std::invalid_argument("Missing option value");
        std::string value = argv[++i];
        if(arg == "--font") result.font = value;
        else if(arg == "--spec-view") result.specView = value;
        else if(arg == "--title") result.title = value;
        else if(arg == "--width") result.width = bounded(value, 160, 16384, "width must be 160..16384");
        else if(arg == "--height") result.height = bounded(value, 120, 16384, "height must be 120..16384");
        else if(arg == "--font-size") result.fontSize = bounded(value, 8, 96, "font-size must be 8..96");
        else if(arg == "--seconds") result.seconds = bounded(value, 1, 3600, "seconds must be 1..3600");
        else throw std::invalid_argument("Unknown option");
    }
    if(result.font.empty()) throw std::invalid_argument("--font <TrueType file> is required");
    // One content source per run, like the overlay host. Stage 1 renders the Pf spec view.
    if(result.specView.empty()) throw std::invalid_argument("--spec-view <file> is required");
    return result;
}
