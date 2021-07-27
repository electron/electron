// Copyright (c) 2021 Microsoft, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/ui/cocoa/window_buttons_proxy.h"

#include "base/i18n/rtl.h"
#include "base/notreached.h"

@implementation ButtonsAreaHoverView : NSView

- (id)initWithProxy:(WindowButtonsProxy*)proxy {
  if ((self = [super init])) {
    proxy_ = proxy;
  }
  return self;
}

// Ignore all mouse events.
- (NSView*)hitTest:(NSPoint)aPoint {
  return nil;
}

- (void)updateTrackingAreas {
  [proxy_ updateTrackingAreas];
}

@end

@implementation WindowButtonsProxy

- (id)initWithWindow:(NSWindow*)window {
  window_ = window;
  show_on_hover_ = NO;
  mouse_inside_ = NO;

  // Save the sequence of the buttons for later computation.
  if (base::i18n::IsRTL()) {
    left_ = [window_ standardWindowButton:NSWindowZoomButton];
    right_ = [window_ standardWindowButton:NSWindowCloseButton];
  } else {
    left_ = [window_ standardWindowButton:NSWindowCloseButton];
    right_ = [window_ standardWindowButton:NSWindowZoomButton];
  }
  middle_ = [window_ standardWindowButton:NSWindowMiniaturizeButton];

  // Safety check just in case Apple changes the view structure in a macOS
  // upgrade.
  if (!left_.superview || !left_.superview.superview) {
    NOTREACHED() << "macOS has changed its window buttons view structure.";
    titlebar_container_ = nullptr;
    return self;
  }
  titlebar_container_ = left_.superview.superview;

  // Remember the default margin.
  margin_ = default_margin_ = [self getCurrentMargin];

  return self;
}

- (void)dealloc {
  if (hover_view_)
    [hover_view_ removeFromSuperview];
  [super dealloc];
}

- (void)setVisible:(BOOL)visible {
  if (!titlebar_container_)
    return;
  [titlebar_container_ setHidden:!visible];
}

- (BOOL)isVisible {
  if (!titlebar_container_)
    return YES;
  return ![titlebar_container_ isHidden];
}

- (void)setShowOnHover:(BOOL)yes {
  if (!titlebar_container_)
    return;
  show_on_hover_ = yes;
  // Put a transparent view above the window buttons so we can track mouse
  // events when mouse enter/leave the window buttons.
  if (show_on_hover_) {
    hover_view_.reset([[ButtonsAreaHoverView alloc] initWithProxy:self]);
    [hover_view_ setFrame:[self getButtonsBounds]];
    [titlebar_container_ addSubview:hover_view_.get()];
  } else {
    [hover_view_ removeFromSuperview];
    hover_view_.reset();
  }
  [self updateButtonsVisibility];
}

- (void)setMargin:(const absl::optional<gfx::Point>&)margin {
  if (margin)
    margin_ = *margin;
  else
    margin_ = default_margin_;
  [self redraw];
}

- (NSRect)getButtonsContainerBounds {
  return NSInsetRect([self getButtonsBounds], -margin_.x(), -margin_.y());
}

- (void)redraw {
  if (!titlebar_container_)
    return;

  float button_width = NSWidth(left_.frame);
  float button_height = NSHeight(left_.frame);
  float padding = NSMinX(middle_.frame) - NSMaxX(left_.frame);
  float start;
  if (base::i18n::IsRTL())
    start =
        NSWidth(window_.frame) - 3 * button_width - 2 * padding - margin_.x();
  else
    start = margin_.x();

  NSRect cbounds = titlebar_container_.frame;
  cbounds.size.height = button_height + 2 * margin_.y();
  cbounds.origin.y = NSHeight(window_.frame) - NSHeight(cbounds);
  [titlebar_container_ setFrame:cbounds];

  [left_ setFrameOrigin:NSMakePoint(start, margin_.y())];
  start += button_width + padding;
  [middle_ setFrameOrigin:NSMakePoint(start, margin_.y())];
  start += button_width + padding;
  [right_ setFrameOrigin:NSMakePoint(start, margin_.y())];

  if (hover_view_)
    [hover_view_ setFrame:[self getButtonsBounds]];
}

- (void)updateTrackingAreas {
  if (tracking_area_)
    [hover_view_ removeTrackingArea:tracking_area_.get()];
  tracking_area_.reset([[NSTrackingArea alloc]
      initWithRect:NSZeroRect
           options:NSTrackingMouseEnteredAndExited | NSTrackingActiveAlways |
                   NSTrackingInVisibleRect
             owner:self
          userInfo:nil]);
  [hover_view_ addTrackingArea:tracking_area_.get()];
}

- (void)mouseEntered:(NSEvent*)event {
  mouse_inside_ = YES;
  [self updateButtonsVisibility];
}

- (void)mouseExited:(NSEvent*)event {
  mouse_inside_ = NO;
  [self updateButtonsVisibility];
}

- (void)updateButtonsVisibility {
  NSArray* buttons = @[
    [window_ standardWindowButton:NSWindowCloseButton],
    [window_ standardWindowButton:NSWindowMiniaturizeButton],
    [window_ standardWindowButton:NSWindowZoomButton],
  ];
  // Show buttons when mouse hovers above them.
  BOOL hidden = show_on_hover_ && !mouse_inside_;
  // Always show buttons under fullscreen.
  if ([window_ styleMask] & NSWindowStyleMaskFullScreen)
    hidden = NO;
  for (NSView* button in buttons) {
    [button setHidden:hidden];
    [button setNeedsDisplay:YES];
  }
}

// Return the bounds of all 3 buttons.
- (NSRect)getButtonsBounds {
  return NSMakeRect(NSMinX(left_.frame), NSMinY(left_.frame),
                    NSMaxX(right_.frame) - NSMinX(left_.frame),
                    NSHeight(left_.frame));
}

// Compute margin from position of current buttons.
- (gfx::Point)getCurrentMargin {
  gfx::Point result;
  if (!titlebar_container_)
    return result;

  result.set_y((NSHeight(titlebar_container_.frame) - NSHeight(left_.frame)) /
               2);

  if (base::i18n::IsRTL())
    result.set_x(NSWidth(window_.frame) - NSMaxX(right_.frame));
  else
    result.set_x(NSMinX(left_.frame));
  return result;
}

@end
