// @spec SPEC-TL-DESKTOP-OVERLAY
#pragma once
#import <AppKit/AppKit.h>

namespace tela::macos {
// Turns a Tela panel into part of a desktop surface: above ordinary and floating windows, on
// every Space and beside full-screen apps, left out of Mission Control's window spread, the
// window cycle and the Window menu. With exclude_from_capture its contents are kept out of
// screenshots, screen recording and screen sharing (NSWindowSharingNone).
void float_on_desktop(NSPanel*, bool exclude_from_capture);
}
