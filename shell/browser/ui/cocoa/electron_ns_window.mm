// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/ui/cocoa/electron_ns_window.h"

#include "base/strings/sys_string_conversions.h"
#include "shell/browser/native_window_mac.h"
#include "shell/browser/ui/cocoa/electron_preview_item.h"
#include "shell/browser/ui/cocoa/electron_touch_bar.h"
#include "shell/browser/ui/cocoa/root_view_mac.h"
#include "ui/base/cocoa/window_size_constants.h"

namespace electron {

int ScopedDisableResize::disable_resize_ = 0;

}  // namespace electron

@interface NSWindow (PrivateAPI)
- (NSImage*)_cornerMask;
@end

@implementation ElectronNSWindow

@synthesize acceptsFirstMouse;
@synthesize enableLargerThanScreen;
@synthesize disableAutoHideCursor;
@synthesize disableKeyOrMainWindow;
@synthesize vibrantView;
@synthesize cornerMask;

- (id)initWithShell:(electron::NativeWindowMac*)shell
          styleMask:(NSUInteger)styleMask {
  if ((self = [super initWithContentRect:ui::kWindowSizeDeterminedLater
                               styleMask:styleMask
                                 backing:NSBackingStoreBuffered
                                   defer:NO])) {
    shell_ = shell;
  }
  return self;
}

- (electron::NativeWindowMac*)shell {
  return shell_;
}

- (id)accessibilityFocusedUIElement {
  views::Widget* widget = shell_->widget();
  id superFocus = [super accessibilityFocusedUIElement];
  if (!widget || shell_->IsFocused())
    return superFocus;
  return nil;
}
- (NSRect)originalContentRectForFrameRect:(NSRect)frameRect {
  return [super contentRectForFrameRect:frameRect];
}

- (NSTouchBar*)makeTouchBar {
  if (shell_->touch_bar())
    return [shell_->touch_bar() makeTouchBar];
  else
    return nil;
}

// NSWindow overrides.

- (void)swipeWithEvent:(NSEvent*)event {
  if (event.deltaY == 1.0) {
    shell_->NotifyWindowSwipe("up");
  } else if (event.deltaX == -1.0) {
    shell_->NotifyWindowSwipe("right");
  } else if (event.deltaY == -1.0) {
    shell_->NotifyWindowSwipe("down");
  } else if (event.deltaX == 1.0) {
    shell_->NotifyWindowSwipe("left");
  }
}

- (void)rotateWithEvent:(NSEvent*)event {
  shell_->NotifyWindowRotateGesture(event.rotation);
}

- (NSRect)contentRectForFrameRect:(NSRect)frameRect {
  if (shell_->has_frame())
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
    if (shell_->has_frame()) {
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
  NSMutableArray* mutableChildren = [[children mutableCopy] autorelease];
  [mutableChildren filterUsingPredicate:predicate];

  return mutableChildren;
}

- (NSString*)accessibilityTitle {
  return base::SysUTF8ToNSString(shell_->GetTitle());
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
    return shell_->IsClosable();
  return [super validateUserInterfaceItem:item];
}

// By overriding this built-in method the corners of the vibrant view (if set)
// will be smooth.
- (NSImage*)_cornerMask {
  if (self.vibrantView != nil) {
    return [self cornerMask];
  } else {
    return [super _cornerMask];
  }
}

// Quicklook methods

- (BOOL)acceptsPreviewPanelControl:(QLPreviewPanel*)panel {
  return YES;
}

- (void)beginPreviewPanelControl:(QLPreviewPanel*)panel {
  panel.delegate = [self delegate];
  panel.dataSource = static_cast<id<QLPreviewPanelDataSource>>([self delegate]);
}

- (void)endPreviewPanelControl:(QLPreviewPanel*)panel {
  panel.delegate = nil;
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

- (void)toggleFullScreenMode:(id)sender {
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
}

- (void)performMiniaturize:(id)sender {
  if (shell_->title_bar_style() ==
      electron::NativeWindowMac::TitleBarStyle::kCustomButtonsOnHover)
    [self miniaturize:self];
  else
    [super performMiniaturize:sender];
}

@end
