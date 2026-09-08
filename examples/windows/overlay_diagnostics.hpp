#pragma once
#include <chrono>
#include <cstdint>
#include <string>
#include "overlay_session.hpp"
// @spec Overlay lifecycle
class OverlayDiagnostics {
public:
    OverlayDiagnostics();
    void write(const std::string& path, const OverlayRunReport& result) const;
private:
    std::chrono::steady_clock::time_point started_;
    std::uint64_t cpu_;
    unsigned long handles_, gdi_;
};
