#include "Tray.h"
#import <AppKit/AppKit.h>
#include <QWindow>

@interface KCCTrayTarget : NSObject
@property(nonatomic, copy) void (^openBlock)(void);
@property(nonatomic, copy) void (^quitBlock)(void);
- (void)openWindow:(id)sender;
- (void)quitApp:(id)sender;
@property(nonatomic, copy) void (^pickBlock)(NSString*);
- (void)pickProfile:(NSMenuItem*)sender;
@end
@implementation KCCTrayTarget
- (void)openWindow:(id)sender { if (self.openBlock) self.openBlock(); }
- (void)quitApp:(id)sender { if (self.quitBlock) self.quitBlock(); }
- (void)pickProfile:(NSMenuItem*)sender {
    // The handler rebuilds this very menu (and replaces pickBlock) while it runs: keep both alive until it returns.
    void (^blk)(NSString*) = [self.pickBlock copy];
    NSString* name = [sender.representedObject copy];
    if (blk) blk(name);
    [blk release]; [name release];
}
@end

@interface KCCCloseTarget : NSObject
@property(nonatomic, copy) void (^hideBlock)(void);
- (void)hideWindow:(id)sender;
@end
@implementation KCCCloseTarget
- (void)hideWindow:(id)sender { if (self.hideBlock) self.hideBlock(); }
@end
static KCCCloseTarget* gClose = nil;

static NSStatusItem* gItem = nil;
static NSMenuItem* gStatus = nil;
static KCCTrayTarget* gTarget = nil;

namespace kcc {
bool trayAvailable() { return true; }

void installTray(const QByteArray& png, const QString& title, TrayActions a) {
    if (gItem) return;
    gTarget = [KCCTrayTarget new];
    auto open = a.open, quit = a.quit;
    gTarget.openBlock = ^{ if (open) open(); };
    gTarget.quitBlock = ^{ if (quit) quit(); };

    // This file is built without ARC: the status item comes back autoreleased and would vanish with the pool.
    gItem = [[[NSStatusBar systemStatusBar] statusItemWithLength:NSSquareStatusItemLength] retain];
    // A stable name matters: the default autosave name "Item-0" is shared by every app's first status item, so a
    // user who once removed any app's icon from the menu bar would hide ours too.
    gItem.autosaveName = @"KCCControlCenterStatusItem";   // own name: the default "Item-0" is shared by every app's first item
    NSData* data = [NSData dataWithBytes:png.constData() length:NSUInteger(png.size())];
    NSImage* img = [[NSImage alloc] initWithData:data];
    img.size = NSMakeSize(18, 18);          // the PNG is 72 px: sharp on any display
    [img setTemplate:YES];
    gItem.button.image = img;
    gItem.button.toolTip = title.toNSString();

    NSMenu* menu = [NSMenu new];
    gStatus = [[NSMenuItem alloc] initWithTitle:@"" action:nil keyEquivalent:@""];
    gStatus.enabled = NO;
    [menu addItem:gStatus];
    [menu addItem:[NSMenuItem separatorItem]];
    NSMenuItem* open1 = [[NSMenuItem alloc] initWithTitle:[@"Open " stringByAppendingString:title.toNSString()]
                                                   action:@selector(openWindow:) keyEquivalent:@""];
    open1.target = gTarget;
    [menu addItem:open1];
    [menu addItem:[NSMenuItem separatorItem]];
    NSMenuItem* quit1 = [[NSMenuItem alloc] initWithTitle:@"Quit" action:@selector(quitApp:) keyEquivalent:@"q"];
    quit1.target = gTarget;
    [menu addItem:quit1];
    gItem.menu = menu;
}

void setTrayProfiles(const QStringList& names, const QString& active, std::function<void(const QString&)> onPick) {
    if (!gItem || !gTarget) return;
    NSMenu* menu = gItem.menu;
    for (NSInteger i = menu.numberOfItems - 1; i >= 0; --i) if ([menu itemAtIndex:i].tag == 777) [menu removeItemAtIndex:i];
    gTarget.pickBlock = ^(NSString* n) { if (onPick) onPick(QString::fromNSString(n)); };
    NSInteger at = 2;   // after the status line and its separator
    NSMenuItem* head = [[NSMenuItem alloc] initWithTitle:@"Profiles" action:nil keyEquivalent:@""];
    head.enabled = NO; head.tag = 777;
    [menu insertItem:head atIndex:at++];
    for (const QString& n : names) {
        NSMenuItem* it = [[NSMenuItem alloc] initWithTitle:n.toNSString() action:@selector(pickProfile:) keyEquivalent:@""];
        it.target = gTarget; it.representedObject = n.toNSString(); it.tag = 777;
        it.state = n == active ? NSControlStateValueOn : NSControlStateValueOff;
        [menu insertItem:it atIndex:at++];
    }
    NSMenuItem* sep = [NSMenuItem separatorItem]; sep.tag = 777;
    [menu insertItem:sep atIndex:at];
}

void setTrayStatus(const QString& line) {
    if (gStatus) gStatus.title = line.toNSString();
}

void setDockIconVisible(bool visible) {
    [NSApp setActivationPolicy:visible ? NSApplicationActivationPolicyRegular : NSApplicationActivationPolicyAccessory];
}

void bringAppToFront() { [NSApp unhide:nil]; [NSApp activateIgnoringOtherApps:YES]; }
void hideApp() { [NSApp hide:nil]; }

void installCloseToHide(QWindow* window, std::function<void()> hide) {
    NSView* view = reinterpret_cast<NSView*>(window->winId());
    NSButton* b = [view.window standardWindowButton:NSWindowCloseButton];
    if (!b) return;
    gClose = [KCCCloseTarget new];
    gClose.hideBlock = ^{ if (hide) hide(); };
    b.target = gClose;
    b.action = @selector(hideWindow:);
}
}
