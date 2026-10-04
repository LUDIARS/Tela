// @spec SPEC-TL-DESKTOP-OVERLAY
#include "desktop_screens.hpp"
#import <AppKit/AppKit.h>

namespace tela::macos {
DesktopScreens desktop_screens() {
    DesktopScreens result;
    NSArray<NSScreen*>* screens = NSScreen.screens;
    if (screens.count == 0) return result;
    result.primary_top = NSMaxY(screens.firstObject.frame);
    bool first = true;
    for (NSScreen* screen in screens) {
        // visibleFrame leaves out the menu bar and the Dock: the work area.
        const NSRect area = screen.visibleFrame;
        result.displays.push_back({area.origin.x, area.origin.y, area.size.width, area.size.height,
            static_cast<float>(screen.backingScaleFactor), first});
        first = false;
    }
    return result;
}
}
