// Copyright (c) 2026 Electron contributors
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/ui/cocoa/fullscreen_titlebar_tracker.h"

#include <utility>

#include "shell/browser/ui/cocoa/window_buttons_proxy.h"

@interface FullscreenTitlebarTracker () {
  __weak NSWindow* window_;
  NSTitlebarAccessoryViewController* accessory_;
  NSMutableArray<id>* observers_;
  __weak NSWindow* toolbar_window_;
  __weak NSView* container_;
  WindowButtonsProxy* buttons_proxy_;
  BOOL container_posts_frames_;
  BOOL active_;
  BOOL stopped_;
  BOOL updating_;
  BOOL observing_reveal_;
  NSRect native_frame_;
  double reveal_;
  base::RepeatingClosure on_change_;
}
- (void)removeGeometryObservers;
@end

@implementation FullscreenTitlebarTracker

- (instancetype)initWithWindow:(NSWindow*)window
                  buttonsProxy:(WindowButtonsProxy*)buttonsProxy
                      onChange:(base::RepeatingClosure)onChange {
  if ((self = [super init])) {
    window_ = window;
    buttons_proxy_ = buttonsProxy;
    on_change_ = std::move(onChange);
    observers_ = [NSMutableArray array];
    accessory_ = [[NSTitlebarAccessoryViewController alloc] init];
    accessory_.view = [[NSView alloc] initWithFrame:NSZeroRect];
    accessory_.hidden = YES;
    // `revealAmount` is a private API on NSTitlebarAccessoryViewController
    // that tells us by what amount the titlebar has been revealed. It allows
    // us to respond to the titlebar being hidden and shown in fullscreen.
    if (![accessory_ respondsToSelector:NSSelectorFromString(@"revealAmount")])
      return self;
    [accessory_ addObserver:self
                 forKeyPath:@"revealAmount"
                    options:NSKeyValueObservingOptionNew
                    context:nullptr];
    observing_reveal_ = YES;
    [window addTitlebarAccessoryViewController:accessory_];
  }
  return self;
}

- (void)observeValueForKeyPath:(NSString*)keyPath
                      ofObject:(id)object
                        change:(NSDictionary*)change
                       context:(void*)context {
  [self update];
}

- (void)enterFullscreen {
  if (stopped_ || !(window_.styleMask & NSWindowStyleMaskFullScreen))
    return;
  active_ = YES;
  [self update];
}

- (void)leaveFullscreen {
  active_ = NO;
  [self update];
}

- (BOOL)isActive {
  return active_ && observing_reveal_;
}

- (NSView*)container {
  return container_;
}

- (NSRect)nativeFrame {
  return native_frame_;
}

- (double)revealAmount {
  return reveal_;
}

- (void)removeGeometryObservers {
  for (id observer in observers_)
    [NSNotificationCenter.defaultCenter removeObserver:observer];
  [observers_ removeAllObjects];
  container_.postsFrameChangedNotifications = container_posts_frames_;
  toolbar_window_ = nil;
  container_ = nil;
}

- (void)update {
  if (stopped_ || updating_ || !observing_reveal_)
    return;
  updating_ = YES;
  NSWindow* window = window_;
  NSView* container = [buttons_proxy_ titleBarContainer];
  NSWindow* toolbar_window = container.window;
  if (!active_ || !(window.styleMask & NSWindowStyleMaskFullScreen) ||
      !toolbar_window || toolbar_window == window) {
    container = nil;
    toolbar_window = nil;
  }
  const bool container_changed =
      container_ != container || toolbar_window_ != toolbar_window;
  if (container_changed) {
    [self removeGeometryObservers];
    container_ = container;
    toolbar_window_ = toolbar_window;
    if (container) {
      __weak FullscreenTitlebarTracker* weak_self = self;
      void (^geometry_changed)(NSNotification*) = ^(NSNotification*) {
        [weak_self update];
      };
      NSNotificationCenter* center = NSNotificationCenter.defaultCenter;
      [observers_
          addObject:[center addObserverForName:NSWindowDidMoveNotification
                                        object:toolbar_window
                                         queue:nil
                                    usingBlock:geometry_changed]];
      container_posts_frames_ = container.postsFrameChangedNotifications;
      container.postsFrameChangedNotifications = YES;
      // On notched displays AppKit moves the view within a stationary window.
      [observers_
          addObject:[center addObserverForName:NSViewFrameDidChangeNotification
                                        object:container
                                         queue:nil
                                    usingBlock:geometry_changed]];
    }
  }
  NSRect frame = NSZeroRect;
  double reveal = 0;
  if (container) {
    frame = [toolbar_window
        convertRectToScreen:[container convertRect:container.bounds
                                            toView:nil]];
    reveal = [[accessory_ valueForKey:@"revealAmount"] doubleValue];
  }
  const bool changed = container_changed ||
                       !NSEqualRects(frame, native_frame_) || reveal != reveal_;
  native_frame_ = frame;
  reveal_ = reveal;
  updating_ = NO;
  if (changed) {
    [[maybe_unused]] FullscreenTitlebarTracker* tracker
        __attribute__((objc_precise_lifetime)) = self;
    base::RepeatingClosure on_change = on_change_;
    on_change.Run();
  }
}

- (void)stop {
  if (stopped_)
    return;
  stopped_ = YES;
  active_ = NO;
  [self removeGeometryObservers];
  if (observing_reveal_) {
    [accessory_ removeObserver:self forKeyPath:@"revealAmount"];
    observing_reveal_ = NO;
  }
  [accessory_ removeFromParentViewController];
  on_change_.Reset();
}

@end
