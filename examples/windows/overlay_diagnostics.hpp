#pragma once
#include <chrono>
#include <cstdint>
#include <string>
// @spec Overlay lifecycle
class OverlayDiagnostics {
public:
    OverlayDiagnostics();
    void write(const std::string& path, std::uint64_t frames) const;
private:
    std::chrono::steady_clock::time_point started_;
    std::uint64_t cpu_;
    unsigned long handles_, gdi_;
};
