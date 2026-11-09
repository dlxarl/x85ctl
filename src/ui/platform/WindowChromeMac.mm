#include "WindowChrome.h"
#include <QWindow>
#import <AppKit/AppKit.h>

namespace kcc {
static void placeButtons(NSWindow* w, CGFloat barHeight, CGFloat leftInset) {
    NSButton* close = [w standardWindowButton:NSWindowCloseButton];
    if (!close || (w.styleMask & NSWindowStyleMaskFullScreen)) return;
    NSView* container = close.superview.superview;   // NSTitlebarContainerView
    if (!container) return;
    // Already in place? (called on every window update, so keep it cheap and loop-free)
    const CGFloat wantY = (barHeight - close.frame.size.height) / 2;
    if (NSHeight(container.frame) == barHeight && close.frame.origin.x == leftInset && close.frame.origin.y == wantY
        && NSHeight(close.superview.frame) == barHeight) return;
    // Grow the title bar container to the app's bar height so the buttons can sit in its middle.
    NSRect r = container.frame;
    r.size.height = barHeight;
    r.origin.y = NSHeight(w.frame) - barHeight;
    container.frame = r;
    close.superview.frame = container.bounds;   // the inner title bar view must span it too, or the buttons stay near the top
    const NSWindowButton kinds[] = {NSWindowCloseButton, NSWindowMiniaturizeButton, NSWindowZoomButton};
    CGFloat x = leftInset;
    for (NSWindowButton k : kinds) {
        NSButton* b = [w standardWindowButton:k];
        const NSSize s = b.frame.size;
        [b setFrameOrigin:NSMakePoint(x, (barHeight - s.height) / 2)];
        x += s.width + 6;
    }
}

void applyWindowChrome(QWindow* window, int barHeight, int leftInset) {
    if (!window) return;
    NSView* view = reinterpret_cast<NSView*>(window->winId());
    NSWindow* w = view.window;
    if (!w) return;
    w.styleMask |= NSWindowStyleMaskFullSizeContentView;
    w.titlebarAppearsTransparent = YES;
    w.titleVisibility = NSWindowTitleHidden;
    if (qEnvironmentVariableIsSet("KCC_CHROME_DEBUG"))
        NSLog(@"chrome: view=%@ contentView=%@ same=%d viewFrame=%@ contentBounds=%@ layoutRect=%@", view, w.contentView,
              view == w.contentView, NSStringFromRect(view.frame), NSStringFromRect(w.contentView.bounds),
              NSStringFromRect(w.contentLayoutRect));
    const CGFloat h = barHeight, left = leftInset;
    placeButtons(w, h, left);
    for (int ms : {0, 100, 400, 1000})
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, int64_t(ms) * NSEC_PER_MSEC), dispatch_get_main_queue(),
                       ^{ if (!(w.styleMask & NSWindowStyleMaskFullScreen)) placeButtons(w, h, left); });
    if (qEnvironmentVariableIsSet("KCC_CHROME_DEBUG")) {
        NSView* frameView = w.contentView.superview;
        for (CGFloat x : {30.0, 500.0, NSWidth(w.frame) - 80})
            NSLog(@"chrome: hit(%.0f, top-26) -> %@", x, [frameView hitTest:NSMakePoint(x, NSHeight(w.frame) - 26)]);
    }
    // AppKit re-lays out the title bar on resize / full screen; put the buttons back each time.
    // AppKit also re-lays the title bar out on its own (first show, key/main changes, appearance), so the
    // buttons can jump back to the default spot at launch: check on every window update as well.
    for (NSNotificationName n : {NSWindowDidResizeNotification, NSWindowDidEndLiveResizeNotification,
                                 NSWindowDidBecomeKeyNotification, NSWindowDidResignKeyNotification,
                                 NSWindowDidBecomeMainNotification, NSWindowDidUpdateNotification,
                                 NSWindowDidChangeScreenNotification}) {
        [[NSNotificationCenter defaultCenter] addObserverForName:n object:w queue:nil
                                                      usingBlock:^(NSNotification*) { placeButtons(w, h, left); }];
    }
    // Leaving full screen: AppKit rebuilds the title bar and can drop our chrome flags, which pushes the
    // content down under a bare strip. Re-assert them (now and once the transition has settled).
    auto reassert = ^{
        w.styleMask |= NSWindowStyleMaskFullSizeContentView;
        w.titlebarAppearsTransparent = YES;
        w.titleVisibility = NSWindowTitleHidden;
        placeButtons(w, h, left);
        [w.contentView setNeedsLayout:YES];
        [w.contentView layoutSubtreeIfNeeded];
    };
    for (NSNotificationName n : {NSWindowWillExitFullScreenNotification, NSWindowDidExitFullScreenNotification}) {
        [[NSNotificationCenter defaultCenter] addObserverForName:n object:w queue:nil usingBlock:^(NSNotification*) {
            reassert();
            dispatch_async(dispatch_get_main_queue(), reassert);
            dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 300 * NSEC_PER_MSEC), dispatch_get_main_queue(), reassert);
        }];
    }
}
}
