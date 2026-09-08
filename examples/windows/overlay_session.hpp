#pragma once
#include "overlay_options.hpp"
#include <cstdint>
#include <tela/presentation_status.hpp>
// @spec Overlay lifecycle
struct OverlayRunReport {
    std::uint64_t frames{};
    tela::PresentationDiagnostics presentation;
};
OverlayRunReport runOverlay(const Options& options);
