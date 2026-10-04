// @spec SPEC-TL-DESKTOP-OVERLAY
#include "desktop_panel.hpp"

namespace tela::macos {
void float_on_desktop(NSPanel* panel, bool exclude_from_capture) {
    // The status level keeps the surface above other apps' floating panels; the panel is still
    // nonactivating (make_panel), so being in front never takes focus.
    panel.level = NSStatusWindowLevel;
    panel.collectionBehavior = NSWindowCollectionBehaviorCanJoinAllSpaces
        | NSWindowCollectionBehaviorFullScreenAuxiliary | NSWindowCollectionBehaviorStationary
        | NSWindowCollectionBehaviorIgnoresCycle;
    panel.excludedFromWindowsMenu = YES;
    panel.becomesKeyOnlyIfNeeded = YES;
    if (exclude_from_capture) panel.sharingType = NSWindowSharingNone;
}
}
