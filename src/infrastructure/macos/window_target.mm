// @spec SPEC-TL-MACOS
#include "window_target.hpp"
#import <CoreGraphics/CoreGraphics.h>
#include <cmath>
#include <stdexcept>

namespace tela::macos {
void require_main_thread() {
    if (![NSThread isMainThread] || !NSApp)
        throw std::logic_error("Tela MacOS requires NSApplication on its main thread");
}
namespace {
CGFloat scale_for(NSRect frame) {
    CGFloat scale = 0, largest = 0;
    for (NSScreen* screen in NSScreen.screens) {
        NSRect overlap = NSIntersectionRect(frame, screen.frame);
        CGFloat area = overlap.size.width * overlap.size.height;
        if (area > largest) { largest = area; scale = screen.backingScaleFactor; }
    }
    return scale;
}
int pixel(CGFloat value) {
    if (!std::isfinite(value) || std::abs(value) > 1000000)
        throw std::runtime_error("Tela MacOS window coordinate out of range");
    return static_cast<int>(std::lround(value));
}
}
WindowTarget observe_window(std::uint32_t window, int expected_process) {
    require_main_thread();
    WindowTarget result;
    result.viewport.host_id = "macos:" + std::to_string(expected_process);
    result.viewport.view_id = std::to_string(window);
    NSArray* rows = CFBridgingRelease(CGWindowListCopyWindowInfo(kCGWindowListOptionIncludingWindow, window));
    if (!rows) throw std::runtime_error("Tela cannot read macOS window metadata");
    if (rows.count == 0) return result;
    NSDictionary* row = rows.firstObject;
    NSNumber* owner = row[(__bridge NSString*)kCGWindowOwnerPID];
    if (!owner) throw std::runtime_error("Tela target window owner unavailable");
    result.process = owner.intValue;
    if (expected_process && result.process != expected_process) return result;
    result.exists = true;
    CGRect bounds{};
    NSDictionary* encoded = row[(__bridge NSString*)kCGWindowBounds];
    if (!encoded || !CGRectMakeWithDictionaryRepresentation((__bridge CFDictionaryRef)encoded, &bounds))
        throw std::runtime_error("Tela target window bounds unavailable");
    if (NSScreen.screens.count == 0) throw std::runtime_error("Tela requires an attached display");
    result.primary_top = NSMaxY(NSScreen.screens.firstObject.frame);
    result.frame = NSMakeRect(bounds.origin.x, result.primary_top - CGRectGetMaxY(bounds), bounds.size.width, bounds.size.height);
    const CGFloat scale = scale_for(result.frame);
    auto& v = result.viewport;
    v.host_id = "macos:" + std::to_string(result.process);
    v.focused = NSWorkspace.sharedWorkspace.frontmostApplication.processIdentifier == result.process;
    v.visible = scale > 0 && v.focused && [row[(__bridge NSString*)kCGWindowIsOnscreen] boolValue];
    if (scale <= 0) return result;
    // macOS global coordinates are points. Use the target display's scale for
    // this viewport's entire coordinate basis, including all pointer events.
    v.dpi_scale = static_cast<float>(scale);
    v.desktop_x = pixel(bounds.origin.x * scale); v.desktop_y = pixel(bounds.origin.y * scale);
    v.width = pixel(bounds.size.width * scale); v.height = pixel(bounds.size.height * scale);
    return result;
}
bool same_geometry(const Viewport& a, const Viewport& b) {
    return a.desktop_x == b.desktop_x && a.desktop_y == b.desktop_y &&
        a.width == b.width && a.height == b.height && a.dpi_scale == b.dpi_scale;
}
}
