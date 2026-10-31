// @implements SPEC-TL-LIST-VIEW
// @spec List view
#include "list_overlay_options.hpp"
#include <stdexcept>

namespace {
constexpr int max_seconds = 86400;

// Whole decimal number within [low, high]; anything else (text, overflow, a fraction) is out of range.
int integer_from(const std::string& value, int low, int high, const char* message) {
    std::size_t used{};
    int number{};
    try { number = std::stoi(value, &used); } catch(const std::exception&) { used = 0; }
    if(used != value.size() || number < low || number > high) throw std::invalid_argument(message);
    return number;
}
}

ListOverlayOptions listOverlayOptions(int argc, char** argv) {
    ListOverlayOptions options;
    for(int i = 1; i < argc; ++i) {
        const std::string name = argv[i];
        if(name == "--exclude-from-capture") { options.exclude_from_capture = true; continue; }
        if(i + 1 >= argc) throw std::invalid_argument("Missing value for " + name);
        const std::string value = argv[++i];
        if(name == "--font") options.font = value;
        else if(name == "--file") options.file = value;
        else if(name == "--corner") options.corner = tela::desktop_corner_from_name(value);
        else if(name == "--width") options.width = static_cast<float>(integer_from(value, 240, 2000, "--width must be 240..2000"));
        else if(name == "--rows") options.rows = static_cast<std::size_t>(integer_from(value, 1, 64, "--rows must be 1..64"));
        else if(name == "--font-size") options.font_size = static_cast<float>(integer_from(value, 8, 96, "--font-size must be 8..96"));
        else if(name == "--seconds") options.seconds = integer_from(value, 1, max_seconds, "--seconds must be 1..86400");
        else throw std::invalid_argument("Unknown option " + name);
    }
    // Without a font nothing can be drawn and without a file there is nothing to show; an empty
    // surface would look like a list with no entries.
    if(options.font.empty()) throw std::invalid_argument("--font <file.ttf> is required");
    if(options.file.empty()) throw std::invalid_argument("--file <list file> is required");
    return options;
}
