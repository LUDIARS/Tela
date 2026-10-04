// @spec SPEC-TL-MACOS
#include "region_panels.hpp"
#include "region_surface.hpp"
#include <tela/input_regions.hpp>
#include <utility>

namespace tela::macos {
RegionPanels::RegionPanels(Configure configure)
    : configure_(std::move(configure)), panels_([NSMutableArray array]) {}
RegionPanels::~RegionPanels() { clear(); }

void RegionPanels::rebuild(Runtime& runtime, PointerSession& pointer) {
    if (runtime.captured()) return; // preserve the native down recipient
    NSMutableArray<NSPanel*>* next = [NSMutableArray array];
    std::vector<std::string> ids;
    try {
        for (const auto& region : exclusive_regions(runtime.elements())) {
            for (const auto fragment : region.fragments) {
                if (fragment.width <= 0 || fragment.height <= 0) continue;
                NSRect bounds = fragment_frame(fragment, pointer.target);
                if (NSIsEmptyRect(bounds)) continue;
                NSPanel* panel = make_input_panel(pointer, bounds);
                [next addObject:panel];
                ids.push_back(region.id);
                if (configure_) configure_(panel);
            }
        }
    } catch (...) { for (NSPanel* panel in next) close_input_panel(panel); throw; }
    clear();
    panels_ = next; ids_ = std::move(ids);
}

void RegionPanels::present(PixelSurface& visual, const WindowTarget& target) {
    present_regions(panels_, visual, target);
}

void RegionPanels::offset(CGFloat dx, CGFloat dy) {
    for (NSPanel* panel in panels_)
        [panel setFrameOrigin:NSMakePoint(panel.frame.origin.x + dx, panel.frame.origin.y + dy)];
}

void RegionPanels::clear() {
    for (NSPanel* panel in panels_) close_input_panel(panel);
    [panels_ removeAllObjects];
    ids_.clear();
}

std::string RegionPanels::region(NSWindow* window) const {
    if (!window) return {};
    const NSUInteger index = [panels_ indexOfObjectIdenticalTo:window];
    return index == NSNotFound ? std::string{} : ids_[index];
}

std::string RegionPanels::under(NSPoint point) const {
    for (NSUInteger index = 0; index < panels_.count; ++index) {
        NSPanel* panel = panels_[index];
        if (panel.isVisible && NSMouseInRect(point, panel.frame, NO)) return ids_[index];
    }
    return {};
}
}
