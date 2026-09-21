// @spec Desktop overlay
#include "desktop_probe_options.hpp"
#include <stdexcept>

namespace {
constexpr int max_seconds = 3600;

int seconds_from(const std::string& value) {
    std::size_t used{};
    int seconds{};
    // A number too large for int is out of range like any other, not a different failure.
    try { seconds = std::stoi(value, &used); } catch(const std::out_of_range&) { used = 0; }
    if(used != value.size() || seconds < 1 || seconds > max_seconds)
        throw std::invalid_argument("--seconds must be 1..3600");
    return seconds;
}
}

DesktopProbeOptions desktopProbeOptions(int argc, char** argv) {
    DesktopProbeOptions options;
    for(int i = 1; i < argc; ++i) {
        const std::string name = argv[i];
        if(i + 1 >= argc) throw std::invalid_argument("Missing value for " + name);
        const std::string value = argv[++i];
        if(name == "--font") options.font = value;
        else if(name == "--corner") options.corner = tela::desktop_corner_from_name(value);
        else if(name == "--seconds") options.seconds = seconds_from(value);
        else throw std::invalid_argument("Unknown option " + name);
    }
    // Without a font nothing can be drawn; an empty surface would look like a passing check.
    if(options.font.empty()) throw std::invalid_argument("--font <file.ttf> is required");
    return options;
}
