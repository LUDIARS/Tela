// @implements SPEC-TL-WEB
// @spec Web rendering
// Check host for Tela::Web without a transport: one input JSON per stdin line, and every
// message WebSession emits (images, frames) on its own stdout line. Errors go to stderr.
//   echo {"type":"resize","width":480,"height":640,"dpi":1} | tela_web_stdio
#include "../controls/controls_gallery.hpp"
#include <tela/web_session.hpp>
#include <iostream>
#include <string>

namespace {
// Reads one line of at most `limit` bytes; a longer line is consumed and reported as too long.
bool read_line(std::istream& in, std::string& line, std::size_t limit, bool& too_long) {
    line.clear();
    too_long = false;
    for (char c; in.get(c);) {
        if (c == '\n') return true;
        if (line.size() < limit) line.push_back(c); else too_long = true;
    }
    return !line.empty() || too_long;
}
void flush(tela::WebSession& session) {
    for (const auto& message : session.poll()) std::cout << message << '\n';
    std::cout.flush();
}
}

int main() {
    try {
        const tela::WebLimits limits;
        tela::Runtime runtime;
        ControlsGallery gallery(runtime);
        tela::WebSession session(runtime, limits);
        std::string line;
        bool too_long = false;
        while (read_line(std::cin, line, limits.input_bytes, too_long)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (line.empty() && !too_long) continue;
            if (too_long) std::cerr << "input rejected: line exceeds " << limits.input_bytes << " bytes\n";
            else if (!session.receive(line)) std::cerr << "input rejected: " << session.last_error() << '\n';
            flush(session);
        }
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Tela web stdio: " << error.what() << '\n';
        return 2;
    }
}
