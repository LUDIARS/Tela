#include <tela/presentation_status.hpp>
// @spec Overlay lifecycle
namespace tela {
PresentationReason presentation_reason(const PresentationObservation& o) noexcept {
    if(!o.target_exists) return PresentationReason::target_lost;
    if(!o.viewport_visible) return PresentationReason::viewport_hidden;
    if(!o.nonempty) return PresentationReason::empty_viewport;
    if(!o.target_visible) return PresentationReason::target_hidden;
    if(o.minimized) return PresentationReason::target_minimized;
    if(!o.target_process || !o.foreground_process) return PresentationReason::foreground_unavailable;
    if(o.target_process!=o.foreground_process) return PresentationReason::foreign_foreground;
    return o.dirty ? PresentationReason::presented : PresentationReason::unchanged;
}
const char* presentation_reason_name(PresentationReason reason) noexcept {
    constexpr const char* names[]{"presented","unchanged","viewport_hidden","empty_viewport",
        "target_lost","target_hidden","target_minimized","foreground_unavailable","foreign_foreground","not_observed"};
    const auto index=static_cast<unsigned>(reason);
    return index<static_cast<unsigned>(PresentationReason::count)?names[index]:"invalid";
}
}
