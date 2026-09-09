// @spec SPEC-TL-MACOS
#include "input_panel.hpp"
#include "bitmap_view.hpp"
#include <cmath>

@interface TelaInputView : NSView
@property(nonatomic, assign) tela::macos::PointerSession* session;
@end
@implementation TelaInputView
- (BOOL)acceptsFirstMouse:(NSEvent*)event { return YES; }
- (void)mouseDown:(NSEvent*)event { if (_session) _session->dispatch(event, tela::PointerPhase::down); }
- (void)mouseDragged:(NSEvent*)event { if (_session) _session->dispatch(event, tela::PointerPhase::move); }
- (void)mouseUp:(NSEvent*)event { if (_session) _session->dispatch(event, tela::PointerPhase::up); }
@end

namespace tela::macos {
void PointerSession::dispatch(NSEvent* event, PointerPhase phase) noexcept {
    try {
        if (NSWorkspace.sharedWorkspace.frontmostApplication.processIdentifier != target.process) {
            runtime.cancel(); return;
        }
        // AppKit sends drag/up to the view receiving down, even outside it.
        // The host retains that panel until release; no event tap or OS replay.
        const NSPoint point = [event.window convertPointToScreen:event.locationInWindow];
        if (phase == PointerPhase::down) ++gesture;
        HostPointerEvent value;
        value.sequence = ++sequence; value.gesture_id = gesture;
        value.viewport_revision = runtime.viewport().revision;
        value.phase = phase; value.button = PointerButton::primary;
        const auto scale = target.viewport.dpi_scale;
        value.desktop_x = static_cast<int>(std::lround(point.x * scale));
        value.desktop_y = static_cast<int>(std::lround((target.primary_top - point.y) * scale));
        runtime.pointer(value, InputSource::native);
    } catch (...) { error = std::current_exception(); runtime.cancel(); }
}
NSPanel* make_input_panel(PointerSession& session, NSRect frame) {
    NSPanel* panel = make_panel(false);
    TelaInputView* view = [[TelaInputView alloc] initWithFrame:NSMakeRect(0,0,frame.size.width,frame.size.height)];
    view.session = &session;
    view.wantsLayer = YES;
    panel.contentView = view;
    [panel setFrame:frame display:NO];
    return panel;
}
void close_input_panel(NSPanel* panel) {
    // An event already queued by AppKit must not dereference the C++ session
    // after the adapter is destroyed, even if it keeps the closed panel alive.
    TelaInputView* view = (TelaInputView*)panel.contentView;
    view.session = nullptr;
    [panel orderOut:nil]; [panel close];
}
}
