#pragma once
// @implements SPEC-TL-CONTROLS
// @spec Controls
#include <tela/runtime.hpp>
#include <string>

// One declaration of every control, shared by the native window and the web stdio host so both
// show the same UI. The gallery owns the values and redeclares after each change, as an
// application using Tela does.
class ControlsGallery {
public:
    explicit ControlsGallery(tela::Runtime&);
    void declare();
private:
    tela::Runtime& runtime_;
    std::string name_, password_, notes_, submitted_;
    float volume_{.5f};
    int selected_{0};
};
