#pragma once
#include <string>
// @spec View host
// A view run is one content source shown in Tela's own window. Size defaults to the
// content's exported size so it is read at 1:1 unless the caller asks otherwise.
struct ViewOptions {
    std::string font, specView, graph, title = "Tela";
    bool fullscreen{};
    int width{}, height{}, fontSize{}, seconds{};
};
ViewOptions viewOptions(int argc, char** argv);
