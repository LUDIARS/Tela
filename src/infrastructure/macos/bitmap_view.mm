// @spec SPEC-TL-MACOS
#include "bitmap_view.hpp"
#import <QuartzCore/QuartzCore.h>
#include <stdexcept>

namespace tela::macos {
NSPanel* make_panel(bool passthrough) {
    NSPanel* panel = [[NSPanel alloc] initWithContentRect:NSMakeRect(0,0,1,1)
        styleMask:NSWindowStyleMaskBorderless | NSWindowStyleMaskNonactivatingPanel
        backing:NSBackingStoreBuffered defer:NO];
    if (!panel) throw std::runtime_error("Cannot create Tela AppKit panel");
    panel.releasedWhenClosed = NO;
    panel.opaque = NO; panel.backgroundColor = NSColor.clearColor;
    panel.hasShadow = NO; panel.hidesOnDeactivate = NO;
    panel.ignoresMouseEvents = passthrough;
    panel.level = NSFloatingWindowLevel;
    panel.collectionBehavior = NSWindowCollectionBehaviorCanJoinAllSpaces | NSWindowCollectionBehaviorFullScreenAuxiliary;
    panel.contentView.wantsLayer = YES;
    return panel;
}
void present_bitmap(NSPanel* panel, const PixelSurface& surface, NSRect frame, CGFloat scale) {
    // CFData copies the surface: the layer can retain the CGImage after render()
    // returns without referencing a destroyed vector.
    CFDataRef data = CFDataCreate(kCFAllocatorDefault, surface.pixels.data(), surface.pixels.size());
    if (!data) throw std::runtime_error("Cannot copy Tela bitmap");
    CGDataProviderRef provider = CGDataProviderCreateWithCFData(data);
    CFRelease(data);
    if (!provider) throw std::runtime_error("Cannot allocate Tela bitmap provider");
    CGColorSpaceRef colors = CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
    if (!colors) { CGDataProviderRelease(provider); throw std::runtime_error("Cannot create Tela color space"); }
    CGImageRef image = CGImageCreate(surface.width, surface.height, 8, 32, surface.width * 4,
        colors, kCGBitmapByteOrder32Little | kCGImageAlphaPremultipliedFirst,
        provider, nullptr, false, kCGRenderingIntentDefault);
    CGColorSpaceRelease(colors); CGDataProviderRelease(provider);
    if (!image) throw std::runtime_error("Cannot create Tela bitmap image");
    [panel setFrame:frame display:NO];
    [CATransaction begin]; [CATransaction setDisableActions:YES];
    panel.contentView.layer.contentsScale = scale;
    panel.contentView.layer.contents = (__bridge id)image;
    [CATransaction commit];
    CGImageRelease(image);
    [panel orderFrontRegardless];
}
}
