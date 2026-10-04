// @implements SPEC-TL-DESKTOP-OVERLAY
// @spec Desktop overlay
// Floats the fixed probe declaration on the macOS desktop so a person can check by eye that it is
// translucent, that the windows below keep their input, that the grip moves it and, with
// --exclude-from-capture, that a screenshot or screen recording leaves it out.
#import <AppKit/AppKit.h>
#include "../desktop/desktop_probe_content.hpp"
#include "../desktop/desktop_probe_options.hpp"
#include <tela/macos_desktop_overlay.hpp>
#include <chrono>
#include <iostream>

namespace {
using Clock = std::chrono::steady_clock;
// The probe polls; the reusable adapter has no timer.
constexpr NSTimeInterval idle_wait = .05;

void run(const DesktopProbeOptions& options) {
    tela::Runtime runtime;
    tela::PictorSurface renderer(options.font);
    DesktopProbeContent content;
    tela::DesktopOverlayOptions surface;
    surface.placement.width = desktop_probe_width;
    surface.placement.height = desktop_probe_height;
    surface.placement.corner = options.corner;
    surface.grip = desktop_probe_grip;
    surface.exclude_from_capture = options.exclude_from_capture;
    surface.moved = [&content](const tela::DesktopPlacement& placement) {
        content.moved(placement);
        // A real caller stores this; the probe prints it so the spot can be checked by hand.
        std::cout << "moved " << placement.x << ' ' << placement.y << ' ' << tela::desktop_corner_name(placement.corner)
                  << ' ' << placement.margin_x << ' ' << placement.margin_y << std::endl;
    };
    tela::MacOSDesktopOverlay overlay(runtime, renderer, std::move(surface));
    std::cout << "window " << overlay.native_window() << (options.exclude_from_capture ? " excluded from capture" : "")
              << std::endl;
    const auto started = Clock::now();
    while (Clock::now() - started < std::chrono::seconds(options.seconds)) {
        @autoreleasepool {
            content.refresh(runtime, overlay.hovered());
            overlay.synchronize();
            NSEvent* event = [NSApp nextEventMatchingMask:NSEventMaskAny
                untilDate:[NSDate dateWithTimeIntervalSinceNow:idle_wait] inMode:NSDefaultRunLoopMode dequeue:YES];
            if (event) [NSApp sendEvent:event];
        }
    }
    overlay.hide();
}
}

int main(int argc, char** argv) {
    @autoreleasepool {
        try {
            const auto options = desktopProbeOptions(argc, argv);
            [NSApplication sharedApplication];
            // Accessory: no Dock icon and no Cmd+Tab entry, as a presenter's overlay should be.
            [NSApp setActivationPolicy:NSApplicationActivationPolicyAccessory];
            [NSApp finishLaunching];
            run(options);
            return 0;
        } catch (const std::exception& error) {
            std::cerr << "Tela macOS desktop overlay probe: " << error.what() << '\n';
            return 2;
        }
    }
}
