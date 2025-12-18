// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/ui/cocoa/electron_ns_window.h"

#include "base/strings/sys_string_conversions.h"
#include "electron/mas.h"
#include "shell/browser/api/electron_api_web_contents.h"
#include "shell/browser/native_window_mac.h"
#include "shell/browser/ui/cocoa/electron_preview_item.h"
#include "shell/browser/ui/cocoa/electron_touch_bar.h"
#include "shell/browser/ui/cocoa/history_overlay_controller.h"
#include "shell/browser/ui/cocoa/root_view_mac.h"
#include "ui/base/cocoa/window_size_constants.h"

#import <objc/message.h>
#import <objc/runtime.h>

using namespace std::string_view_literals;

namespace electron {

int ScopedDisableResize::disable_resize_ = 0;

}  // namespace electron

@interface NSWindow (PrivateAPI)
- (int64_t)_resizeDirectionForMouseLocation:(CGPoint)location;
@end

// See components/remote_cocoa/app_shim/native_widget_mac_nswindow.mm
@interface NSView (CRFrameViewAdditions)
- (void)cr_mouseDownOnFrameView:(NSEvent*)event;
@end

typedef void (*MouseDownImpl)(id, SEL, NSEvent*);

namespace {
MouseDownImpl g_nsthemeframe_mousedown;
MouseDownImpl g_nsnextstepframe_mousedown;
}  // namespace

// This class is never instantiated, it's just a container for our swizzled
// methods.
@interface SwizzledMethodsClass : NSView
@end

@implementation SwizzledMethodsClass
- (void)swiz_nsthemeframe_mouseDown:(NSEvent*)event {
  if ([self.window respondsToSelector:@selector(shell)]) {
    electron::NativeWindowMac* shell =
        (electron::NativeWindowMac*)[(id)self.window shell];
    if (shell && !shell->has_frame())
      [self cr_mouseDownOnFrameView:event];
    g_nsthemeframe_mousedown(self, @selector(mouseDown:), event);
  }
}

- (void)swiz_nsnextstepframe_mouseDown:(NSEvent*)event {
  if ([self.window respondsToSelector:@selector(shell)]) {
    electron::NativeWindowMac* shell =
        (electron::NativeWindowMac*)[(id)self.window shell];
    if (shell && !shell->has_frame()) {
      [self cr_mouseDownOnFrameView:event];
    }
    g_nsnextstepframe_mousedown(self, @selector(mouseDown:), event);
  }
}

- (void)swiz_nsview_swipeWithEvent:(NSEvent*)event {
  if ([self.window respondsToSelector:@selector(shell)]) {
    electron::NativeWindowMac* shell =
        (electron::NativeWindowMac*)[(id)self.window shell];
    if (shell) {
      if (event.deltaY == 1.0) {
        shell->NotifyWindowSwipe("up");
      } else if (event.deltaX == -1.0) {
        shell->NotifyWindowSwipe("right");
      } else if (event.deltaY == -1.0) {
        shell->NotifyWindowSwipe("down");
      } else if (event.deltaX == 1.0) {
        shell->NotifyWindowSwipe("left");
      }
    }
  }
}
@end

namespace {
#if IS_MAS_BUILD()
void SwizzleMouseDown(NSView* frame_view,
                      SEL swiz_selector,
                      MouseDownImpl* orig_impl) {
  Method original_mousedown =
      class_getInstanceMethod([frame_view class], @selector(mouseDown:));
  *orig_impl = (MouseDownImpl)method_getImplementation(original_mousedown);
  Method new_mousedown =
      class_getInstanceMethod([SwizzledMethodsClass class], swiz_selector);
  method_setImplementation(original_mousedown,
                           method_getImplementation(new_mousedown));
}
#else
// components/remote_cocoa/app_shim/bridged_content_view.h overrides
// swipeWithEvent, so we can't just override the implementation
// in ElectronNSWindow like we do with for ex. rotateWithEvent.
void SwizzleSwipeWithEvent(NSView* view, SEL swiz_selector) {
  Method original_swipe_with_event =
      class_getInstanceMethod([view class], @selector(swipeWithEvent:));
  Method new_swipe_with_event =
      class_getInstanceMethod([SwizzledMethodsClass class], swiz_selector);
  method_setImplementation(original_swipe_with_event,
                           method_getImplementation(new_swipe_with_event));
}
#endif

}  // namespace

@implementation ElectronNSWindow {
  // Swipe navigation state
  NSSize _cumulativeScrollDelta;
  BOOL _inSwipeGesture;
  BOOL _potentialSwipeGesture;
  HistoryOverlayController* __strong _historyOverlay;
  id __strong _scrollEventMonitor;
}

@synthesize acceptsFirstMouse;
@synthesize enableLargerThanScreen;
@synthesize disableAutoHideCursor;
@synthesize disableKeyOrMainWindow;
@synthesize vibrantView;

- (id)initWithShell:(electron::NativeWindowMac*)shell
          styleMask:(NSUInteger)styleMask {
  if ((self = [super initWithContentRect:ui::kWindowSizeDeterminedLater
                               styleMask:styleMask
                                 backing:NSBackingStoreBuffered
                                   defer:NO])) {
#if IS_MAS_BUILD()
    // The first time we create a frameless window, we swizzle the
    // implementation of -[NSNextStepFrame mouseDown:], replacing it with our
    // own.
    // This is only necessary on MAS where we can't directly refer to
    // NSNextStepFrame or NSThemeFrame, as they are private APIs.
    // See components/remote_cocoa/app_shim/native_widget_mac_nswindow.mm for
    // the non-MAS-compatible way of doing this.
    if (styleMask & NSWindowStyleMaskTitled) {
      if (!g_nsthemeframe_mousedown) {
        NSView* theme_frame = [[self contentView] superview];
        DCHECK_EQ("NSThemeFrame"sv, class_getName([theme_frame class]));
        SwizzleMouseDown(theme_frame, @selector(swiz_nsthemeframe_mouseDown:),
                         &g_nsthemeframe_mousedown);
      }
    } else {
      if (!g_nsnextstepframe_mousedown) {
        NSView* nextstep_frame = [[self contentView] superview];
        DCHECK_EQ("NSNextStepFrame"sv, class_getName([nextstep_frame class]));
        SwizzleMouseDown(nextstep_frame,
                         @selector(swiz_nsnextstepframe_mouseDown:),
                         &g_nsnextstepframe_mousedown);
      }
    }
#else
    NSView* frameView = [[self contentView] superview];
    SwizzleSwipeWithEvent(frameView, @selector(swiz_nsview_swipeWithEvent:));
#endif  // IS_MAS_BUILD
    shell_ = shell;
    _inSwipeGesture = NO;
    _potentialSwipeGesture = NO;
    _cumulativeScrollDelta = NSZeroSize;

    // Set up scroll event monitor for swipe navigation
    [self setupScrollEventMonitor];
  }
  return self;
}

- (void)setupScrollEventMonitor {
  __weak ElectronNSWindow* weakSelf = self;

  _scrollEventMonitor = [NSEvent
      addLocalMonitorForEventsMatchingMask:NSEventMaskScrollWheel
                                   handler:^NSEvent*(NSEvent* event) {
                                     ElectronNSWindow* strongSelf = weakSelf;
                                     if (!strongSelf)
                                       return event;

                                     // Only handle events for this window
                                     if (event.window != strongSelf)
                                       return event;

                                     // Check if swipe navigation is enabled
                                     if (!strongSelf->shell_ ||
                                         !strongSelf->shell_
                                              ->IsSwipeToNavigateEnabled())
                                       return event;

                                     // Try to handle as swipe
                                     if ([strongSelf
                                             handleSwipeScrollEvent:event]) {
                                       return nil;  // Consume the event
                                     }
                                     return event;
                                   }];
}

- (void)cleanup {
  if (_scrollEventMonitor) {
    [NSEvent removeMonitor:_scrollEventMonitor];
    _scrollEventMonitor = nil;
  }
  [self dismissHistoryOverlay];
  shell_ = nullptr;
}

- (electron::NativeWindowMac*)shell {
  return shell_;
}

- (id)accessibilityFocusedUIElement {
  if (!shell_ || !shell_->widget() || shell_->IsFocused())
    return [super accessibilityFocusedUIElement];
  return nil;
}
- (NSRect)originalContentRectForFrameRect:(NSRect)frameRect {
  return [super contentRectForFrameRect:frameRect];
}

- (NSTouchBar*)makeTouchBar {
  if (shell_ && shell_->touch_bar())
    return [shell_->touch_bar() makeTouchBar];
  else
    return nil;
}

#pragma mark - Swipe Navigation

- (BOOL)canNavigateBack {
  if (!shell_)
    return NO;
  id delegate = [self delegate];
  if ([delegate respondsToSelector:@selector(canNavigateInDirection:
                                                           onWindow:)]) {
    return [delegate canNavigateInDirection:electron::history_swiper::kBackwards
                                   onWindow:self];
  }
  return NO;
}

- (BOOL)canNavigateForward {
  if (!shell_)
    return NO;
  id delegate = [self delegate];
  if ([delegate respondsToSelector:@selector(canNavigateInDirection:
                                                           onWindow:)]) {
    return [delegate canNavigateInDirection:electron::history_swiper::kForwards
                                   onWindow:self];
  }
  return NO;
}

- (void)navigateBack {
  id delegate = [self delegate];
  if ([delegate respondsToSelector:@selector(navigateInDirection:onWindow:)]) {
    [delegate navigateInDirection:electron::history_swiper::kBackwards
                         onWindow:self];
  }
}

- (void)navigateForward {
  id delegate = [self delegate];
  if ([delegate respondsToSelector:@selector(navigateInDirection:onWindow:)]) {
    [delegate navigateInDirection:electron::history_swiper::kForwards
                         onWindow:self];
  }
}

- (void)showHistoryOverlayForDirection:(BOOL)isForward {
  [self dismissHistoryOverlay];
  _historyOverlay = [[HistoryOverlayController alloc]
      initForMode:isForward ? electron::kHistoryOverlayModeForward
                            : electron::kHistoryOverlayModeBack];
  [_historyOverlay showPanelForView:[[self contentView] superview]];
}

- (void)dismissHistoryOverlay {
  if (_historyOverlay) {
    [_historyOverlay dismiss];
    _historyOverlay = nil;
  }
}

- (BOOL)handleSwipeScrollEvent:(NSEvent*)event {
  // Only handle events with proper gesture phases
  NSEventPhase phase = event.phase;
  NSEventPhase momentumPhase = event.momentumPhase;

  // Reset state on new gesture
  if (phase == NSEventPhaseBegan) {
    _cumulativeScrollDelta = NSZeroSize;
    _potentialSwipeGesture = NO;
    _inSwipeGesture = NO;
    return NO;
  }

  // Ignore momentum phase and non-gesture events
  if (momentumPhase != NSEventPhaseNone || phase == NSEventPhaseNone) {
    return NO;
  }

  // If already in a tracked swipe gesture, let it handle
  if (_inSwipeGesture) {
    return NO;
  }

  // Check if swipe tracking from scroll events is enabled in System Prefs
  if (!NSEvent.swipeTrackingFromScrollEventsEnabled) {
    return NO;
  }

  // Accumulate scroll deltas
  _cumulativeScrollDelta.width += event.scrollingDeltaX;
  _cumulativeScrollDelta.height += event.scrollingDeltaY;

  // Wait for enough scroll data to determine direction
  CGFloat absWidth = fabs(_cumulativeScrollDelta.width);
  CGFloat absHeight = fabs(_cumulativeScrollDelta.height);

  // Need minimum movement to start considering
  CGFloat totalMovement = absWidth + absHeight;
  if (totalMovement < 5) {
    return NO;
  }

  // If vertical movement dominates, this is not a swipe
  if (absHeight > absWidth * 0.5) {
    _potentialSwipeGesture = NO;
    return NO;
  }

  // Horizontal movement dominates - potential swipe
  _potentialSwipeGesture = YES;

  // Need enough horizontal movement to trigger
  if (absWidth < 30) {
    return NO;
  }

  // Determine direction - positive deltaX means scrolling content right,
  // which in natural scrolling means swiping left (go back)
  BOOL goBack = _cumulativeScrollDelta.width > 0;
  if (event.directionInvertedFromDevice) {
    goBack = !goBack;
  }

  // Check if navigation is possible in that direction
  BOOL canNavigate =
      goBack ? [self canNavigateBack] : [self canNavigateForward];
  if (!canNavigate) {
    return NO;
  }

  // Start the swipe tracking
  [self initiateSwipeTrackingWithEvent:event isBack:goBack];
  return YES;
}

- (void)initiateSwipeTrackingWithEvent:(NSEvent*)event isBack:(BOOL)isBack {
  _inSwipeGesture = YES;

  __block HistoryOverlayController* overlay = [[HistoryOverlayController alloc]
      initForMode:isBack ? electron::kHistoryOverlayModeBack
                         : electron::kHistoryOverlayModeForward];

  __weak ElectronNSWindow* weakSelf = self;
  __block BOOL didNavigate = NO;

  [event trackSwipeEventWithOptions:NSEventSwipeTrackingLockDirection
           dampenAmountThresholdMin:-1
                                max:1
                       usingHandler:^(CGFloat gestureAmount, NSEventPhase phase,
                                      BOOL isComplete, BOOL* stop) {
                         ElectronNSWindow* strongSelf = weakSelf;
                         if (!strongSelf) {
                           *stop = YES;
                           return;
                         }

                         if (phase == NSEventPhaseBegan) {
                           [overlay showPanelForView:[[strongSelf contentView]
                                                         superview]];
                           return;
                         }

                         // gestureAmount goes from 0 to +/-1
                         // Progress should be absolute value, scaled
                         CGFloat progress = fabs(gestureAmount);
                         BOOL finished = progress >= 0.3;
                         CGFloat displayProgress = fmin(progress / 0.3, 1.0);
                         [overlay setProgress:displayProgress
                                     finished:finished];

                         if (phase == NSEventPhaseEnded && finished &&
                             !didNavigate) {
                           didNavigate = YES;
                           if (isBack) {
                             [strongSelf navigateBack];
                           } else {
                             [strongSelf navigateForward];
                           }
                         }

                         if (phase == NSEventPhaseEnded ||
                             phase == NSEventPhaseCancelled || isComplete) {
                           [overlay dismiss];
                           overlay = nil;
                           strongSelf->_inSwipeGesture = NO;
                           strongSelf->_potentialSwipeGesture = NO;
                           strongSelf->_cumulativeScrollDelta = NSZeroSize;
                         }
                       }];
}

#pragma mark - NSWindow overrides

- (void)sendEvent:(NSEvent*)event {
  // Draggable regions only respond to left-click dragging, but the system will
  // still suppress right-clicks in a draggable region. Temporarily disabling
  // draggable regions allows the underlying views to respond to right-click
  // to potentially bring up a frame context menu.
  BOOL shouldDisableDraggable =
      (event.type == NSEventTypeRightMouseDown ||
       (event.type == NSEventTypeLeftMouseDown &&
        ([event modifierFlags] & NSEventModifierFlagControl)));

  if (shouldDisableDraggable) {
    electron::api::WebContents::SetDisableDraggableRegions(true);
  }

  [super sendEvent:event];

  if (shouldDisableDraggable) {
    electron::api::WebContents::SetDisableDraggableRegions(false);
  }
}

- (void)rotateWithEvent:(NSEvent*)event {
  if (shell_)
    shell_->NotifyWindowRotateGesture(event.rotation);
}

- (NSRect)contentRectForFrameRect:(NSRect)frameRect {
  if (shell_ && shell_->has_frame())
    return [super contentRectForFrameRect:frameRect];
  else
    return frameRect;
}

- (NSRect)constrainFrameRect:(NSRect)frameRect toScreen:(NSScreen*)screen {
  // Resizing is disabled.
  if (electron::ScopedDisableResize::IsResizeDisabled())
    return [self frame];

  NSRect result = [super constrainFrameRect:frameRect toScreen:screen];
  // Enable the window to be larger than screen.
  if ([self enableLargerThanScreen]) {
    // If we have a frame, ensure that we only position the window
    // somewhere where the user can move or resize it (and not
    // behind the menu bar, for instance)
    //
    // If there's no frame, put the window wherever the developer
    // wanted it to go
    if (shell_ && shell_->has_frame()) {
      result.size = frameRect.size;
    } else {
      result = frameRect;
    }
  }

  return result;
}

- (void)setFrame:(NSRect)windowFrame display:(BOOL)displayViews {
  // constrainFrameRect is not called on hidden windows so disable adjusting
  // the frame directly when resize is disabled
  if (!electron::ScopedDisableResize::IsResizeDisabled())
    [super setFrame:windowFrame display:displayViews];
}

- (void)orderWindow:(NSWindowOrderingMode)place relativeTo:(NSInteger)otherWin {
  [self disableHeadlessMode];
  [super orderWindow:place relativeTo:otherWin];
}

- (id)accessibilityAttributeValue:(NSString*)attribute {
  if ([attribute isEqual:NSAccessibilityEnabledAttribute])
    return [NSNumber numberWithBool:YES];
  if (![attribute isEqualToString:@"AXChildren"])
    return [super accessibilityAttributeValue:attribute];

  // We want to remove the window title (also known as
  // NSAccessibilityReparentingCellProxy), which VoiceOver already sees.
  // * when VoiceOver is disabled, this causes Cmd+C to be used for TTS but
  //   still leaves the buttons available in the accessibility tree.
  // * when VoiceOver is enabled, the full accessibility tree is used.
  // Without removing the title and with VO disabled, the TTS would always read
  // the window title instead of using Cmd+C to get the selected text.
  NSPredicate* predicate =
      [NSPredicate predicateWithFormat:@"(self.className != %@)",
                                       @"NSAccessibilityReparentingCellProxy"];

  NSArray* children = [super accessibilityAttributeValue:attribute];
  NSMutableArray* mutableChildren = [children mutableCopy];
  [mutableChildren filterUsingPredicate:predicate];

  return mutableChildren;
}

- (NSString*)accessibilityTitle {
  return base::SysUTF8ToNSString(shell_ ? shell_->GetTitle() : "");
}

- (BOOL)canBecomeMainWindow {
  return !self.disableKeyOrMainWindow;
}

- (BOOL)canBecomeKeyWindow {
  return !self.disableKeyOrMainWindow;
}

- (NSView*)frameView {
  return [[self contentView] superview];
}

- (BOOL)validateUserInterfaceItem:(id<NSValidatedUserInterfaceItem>)item {
  // By default "Close Window" is always disabled when window has no title, to
  // support closing a window without title we need to manually do menu item
  // validation. This code path is used by the "roundedCorners" option.
  if ([item action] == @selector(performClose:))
    return shell_ && shell_->IsClosable();
  return [super validateUserInterfaceItem:item];
}

- (void)disableHeadlessMode {
  if (shell_) {
    // We initialize the window in headless mode to allow painting before it is
    // shown, but we don't want the headless behavior of allowing the window to
    // be placed unconstrained.
    self.isHeadless = false;
    if (shell_->widget())
      shell_->widget()->DisableHeadlessMode();
  }
}

// Quicklook methods

- (BOOL)acceptsPreviewPanelControl:(QLPreviewPanel*)panel {
  return YES;
}

- (void)beginPreviewPanelControl:(QLPreviewPanel*)panel {
  panel.dataSource = static_cast<id<QLPreviewPanelDataSource>>([self delegate]);
}

- (void)endPreviewPanelControl:(QLPreviewPanel*)panel {
  panel.dataSource = nil;
}

// Custom window button methods

- (BOOL)windowShouldClose:(id)sender {
  return YES;
}

- (void)performClose:(id)sender {
  if (shell_->title_bar_style() ==
      electron::NativeWindowMac::TitleBarStyle::kCustomButtonsOnHover) {
    [[self delegate] windowShouldClose:self];
  } else if (!([self styleMask] & NSWindowStyleMaskTitled)) {
    // performClose does not work for windows without title, so we have to
    // emulate its behavior. This code path is used by "simpleFullscreen" and
    // "roundedCorners" options.
    if ([[self delegate] respondsToSelector:@selector(windowShouldClose:)]) {
      if (![[self delegate] windowShouldClose:self])
        return;
    } else if ([self respondsToSelector:@selector(windowShouldClose:)]) {
      if (![self windowShouldClose:self])
        return;
    }
    [self close];
  } else if (shell_->is_modal() && shell_->parent() && shell_->IsVisible()) {
    // We don't want to actually call [window close] here since
    // we've already called endSheet on the modal sheet.
    return;
  } else {
    [super performClose:sender];
  }
}

- (BOOL)toggleFullScreenMode:(id)sender {
  if (!shell_)
    return NO;

  bool is_simple_fs = shell_->IsSimpleFullScreen();
  bool always_simple_fs = shell_->always_simple_fullscreen();

  // If we're in simple fullscreen mode and trying to exit it
  // we need to ensure we exit it properly to prevent a crash
  // with NSWindowStyleMaskTitled mode.
  if (is_simple_fs || always_simple_fs) {
    shell_->SetSimpleFullScreen(!is_simple_fs);
  } else {
    if (shell_->IsVisible()) {
      // Until 10.13, AppKit would obey a call to -toggleFullScreen: made inside
      // windowDidEnterFullScreen & windowDidExitFullScreen. Starting in 10.13,
      // it behaves as though the transition is still in progress and just emits
      // "not in a fullscreen state" when trying to exit fullscreen in the same
      // runloop that entered it. To handle this, invoke -toggleFullScreen:
      // asynchronously.
      [super performSelector:@selector(toggleFullScreen:)
                  withObject:nil
                  afterDelay:0];
    } else {
      [super toggleFullScreen:sender];
    }

    // Exiting fullscreen causes Cocoa to redraw the NSWindow, which resets
    // the enabled state for NSWindowZoomButton. We need to persist it.
    bool maximizable = shell_->IsMaximizable();
    shell_->SetMaximizable(maximizable);
  }

  return YES;
}

- (void)performMiniaturize:(id)sender {
  if (shell_ &&
      shell_->title_bar_style() ==
          electron::NativeWindowMac::TitleBarStyle::kCustomButtonsOnHover) {
    [self miniaturize:self];
  } else {
    [super performMiniaturize:sender];
  }
}

@end
