// @spec SPEC-TL-MACOS
#pragma once
#import <AppKit/AppKit.h>
#include <tela/host_contract.hpp>

namespace tela::macos {
struct WindowTarget {
    NSRect frame{}; // AppKit global points, bottom-left origin
    CGFloat primary_top{};
    Viewport viewport;
    int process{};
    bool exists{};
};
void require_main_thread();
WindowTarget observe_window(std::uint32_t window, int expected_process);
bool same_geometry(const Viewport&, const Viewport&);
}
