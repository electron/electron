// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/ui/cocoa/atom_ns_window.h"

#include "atom/browser/native_window_mac.h"
#include "atom/browser/ui/cocoa/atom_preview_item.h"
#include "atom/browser/ui/cocoa/atom_touch_bar.h"
#include "ui/base/cocoa/window_size_constants.h"

namespace atom {

bool ScopedDisableResize::disable_resize_ = false;

}  // namespace atom

@implementation AtomNSWindow

@synthesize acceptsFirstMouse;
@synthesize enableLargerThanScreen;
@synthesize disableAutoHideCursor;
@synthesize disableKeyOrMainWindow;
@synthesize vibrantView;

- (id)initWithShell:(atom::NativeWindowMac*)shell
          styleMask:(NSUInteger)styleMask {
  if ((self = [super initWithContentRect:ui::kWindowSizeDeterminedLater
                               styleMask:styleMask
                                 backing:NSBackingStoreBuffered
                                   defer:YES])) {
    shell_ = shell;
  }
  return self;
}

- (atom::NativeWindowMac*)shell {
  return shell_;
}

- (NSRect)originalContentRectForFrameRect:(NSRect)frameRect {
  return [super contentRectForFrameRect:frameRect];
}

- (NSTouchBar*)makeTouchBar API_AVAILABLE(macosx(10.12.2)) {
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

- (NSRect)contentRectForFrameRect:(NSRect)frameRect {
  if (shell_->has_frame())
    return [super contentRectForFrameRect:frameRect];
  else
    return frameRect;
}

- (NSRect)constrainFrameRect:(NSRect)frameRect toScreen:(NSScreen*)screen {
  // Resizing is disabled.
  if (atom::ScopedDisableResize::IsResizeDisabled())
    return [self frame];

  // Enable the window to be larger than screen.
  if ([self enableLargerThanScreen])
    return frameRect;
  else
    return [super constrainFrameRect:frameRect toScreen:screen];
}

- (void)setFrame:(NSRect)windowFrame display:(BOOL)displayViews {
  // constrainFrameRect is not called on hidden windows so disable adjusting
  // the frame directly when resize is disabled
  if (!atom::ScopedDisableResize::IsResizeDisabled())
    [super setFrame:windowFrame display:displayViews];
}

- (id)accessibilityAttributeValue:(NSString*)attribute {
  if (![attribute isEqualToString:@"AXChildren"])
    return [super accessibilityAttributeValue:attribute];

  // Filter out objects that aren't the title bar buttons. This has the effect
  // of removing the window title, which VoiceOver already sees.
  // * when VoiceOver is disabled, this causes Cmd+C to be used for TTS but
  //   still leaves the buttons available in the accessibility tree.
  // * when VoiceOver is enabled, the full accessibility tree is used.
  // Without removing the title and with VO disabled, the TTS would always read
  // the window title instead of using Cmd+C to get the selected text.
  NSPredicate* predicate = [NSPredicate
      predicateWithFormat:@"(self isKindOfClass: %@) OR (self.className == %@)",
                          [NSButtonCell class], @"RenderWidgetHostViewCocoa"];

  NSArray* children = [super accessibilityAttributeValue:attribute];
  return [children filteredArrayUsingPredicate:predicate];
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

- (void)performClose:(id)sender {
  if (shell_->title_bar_style() ==
      atom::NativeWindowMac::CUSTOM_BUTTONS_ON_HOVER)
    [[self delegate] windowShouldClose:self];
  else
    [super performClose:sender];
}

- (void)toggleFullScreenMode:(id)sender {
  if (shell_->simple_fullscreen())
    shell_->SetSimpleFullScreen(!shell_->IsSimpleFullScreen());
  else
    [super toggleFullScreen:sender];
}

- (void)performMiniaturize:(id)sender {
  if (shell_->title_bar_style() ==
      atom::NativeWindowMac::CUSTOM_BUTTONS_ON_HOVER)
    [self miniaturize:self];
  else
    [super performMiniaturize:sender];
}

@end
