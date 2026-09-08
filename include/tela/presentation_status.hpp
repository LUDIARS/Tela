#pragma once
#include <array>
#include <cstdint>
// @spec Overlay lifecycle
namespace tela {
enum class PresentationReason { presented, unchanged, viewport_hidden, empty_viewport,
    target_lost, target_hidden, target_minimized, foreground_unavailable, foreign_foreground, not_observed, count };
struct PresentationObservation {
    bool viewport_visible{}, nonempty{}, target_exists{}, target_visible{}, minimized{}, dirty{};
    std::uint32_t target_process{}, foreground_process{};
};
PresentationReason presentation_reason(const PresentationObservation&) noexcept;
const char* presentation_reason_name(PresentationReason) noexcept;
struct PresentationDiagnostics {
    std::array<std::uint64_t, static_cast<unsigned>(PresentationReason::count)> counts{};
    PresentationReason last{PresentationReason::not_observed};
    PresentationObservation observation{};
    void record(PresentationReason reason, PresentationObservation value) noexcept {
        last=reason; observation=value; ++counts[static_cast<unsigned>(reason)];
    }
    void append(const PresentationDiagnostics& other) noexcept {
        for(unsigned i=0;i<counts.size();++i) counts[i]+=other.counts[i];
        if(other.last!=PresentationReason::not_observed) { last=other.last; observation=other.observation; }
    }
};
}
