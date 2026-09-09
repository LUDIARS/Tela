// @spec SPEC-TL-MACOS
#pragma once
#import <AppKit/AppKit.h>
#include <tela/pixel_surface.hpp>
namespace tela::macos {
NSPanel* make_panel(bool passthrough);
void present_bitmap(NSPanel*, const PixelSurface&, NSRect frame, CGFloat scale);
}
