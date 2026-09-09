// @spec SPEC-TL-MACOS
#import <AppKit/AppKit.h>
#include <tela/macos_overlay.hpp>
#include <tela/callout.hpp>
#include <charconv>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <string_view>

namespace {
unsigned integer(std::string_view text) {
    unsigned value{};
    const auto [end, error] = std::from_chars(text.data(), text.data()+text.size(), value);
    if (error != std::errc{} || end != text.data()+text.size() || !value)
        throw std::invalid_argument("Expected a positive decimal integer");
    return value;
}
tela::Document declare_tools(const tela::Viewport& view, int& clicks) {
    tela::Document document;
    document.button("count", "Tela: " + std::to_string(clicks), [&clicks] { ++clicks; });
    tela::Callout label;
    label.anchor = {view.width / view.dpi_scale * .7f, view.height / view.dpi_scale * .65f};
    label.visible = view.visible;
    tela::callout(document, "target.name", "TARGET", label,
        {0,0,view.width / view.dpi_scale,view.height / view.dpi_scale});
    return document;
}
void run(std::uint32_t target, const char* font, unsigned seconds) {
    tela::Runtime runtime;
    tela::PictorSurface renderer(font);
    tela::MacOSOverlay overlay(runtime, renderer, target);
    int clicks = 0, last_clicks = -1;
    std::uint64_t revision = 0;
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(seconds);
    while (std::chrono::steady_clock::now() < deadline) {
        @autoreleasepool {
            overlay.synchronize();
            if (revision != runtime.viewport().revision || clicks != last_clicks) {
                runtime.document(declare_tools(runtime.viewport(), clicks));
                revision = runtime.viewport().revision; last_clicks = clicks;
                overlay.synchronize();
            }
            // The sample polls metadata; the reusable adapter has no timer.
            NSDate* until = [NSDate dateWithTimeIntervalSinceNow:runtime.viewport().visible ? .05 : .25];
            NSEvent* event = [NSApp nextEventMatchingMask:NSEventMaskAny untilDate:until
                inMode:NSDefaultRunLoopMode dequeue:YES];
            if (event) [NSApp sendEvent:event];
        }
    }
    overlay.hide();
}
}
int main(int argc, char** argv) {
    @autoreleasepool {
        try {
            if (argc < 3 || argc > 4) throw std::invalid_argument("Usage: tela_macos_overlay <CGWindowID> <font.ttf> [seconds<=3600]");
            const auto target = integer(argv[1]);
            const auto seconds = argc == 4 ? integer(argv[3]) : 120;
            if (seconds > 3600) throw std::invalid_argument("Duration exceeds 3600 seconds");
            [NSApplication sharedApplication];
            [NSApp setActivationPolicy:NSApplicationActivationPolicyAccessory];
            [NSApp finishLaunching];
            run(target, argv[2], seconds);
            return 0;
        } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
    }
}
