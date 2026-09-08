#pragma once
#include <tela/runtime.hpp>
#include <optional>
// @spec Overlay drawing
// The sample coordinator owns declaration refresh; actions only change state.
class ProbeContent {
public:
    bool refresh_from_target(tela::Runtime& runtime,std::uintptr_t target,int& clicks);
    void refresh(tela::Runtime& runtime,int& clicks);
private:
    // Keyed on what the declaration actually reads, not on the viewport revision:
    // Runtime::disconnect() resets that counter to zero, so it repeats values across
    // a target loss and would otherwise suppress a needed redeclaration.
    struct Revision {
        int width, height;
        float dpi_scale;
        bool visible;
        int clicks;
        bool operator==(const Revision&) const = default;
    };
    std::optional<Revision> declared_;
};
