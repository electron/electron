// Copyright (c) 2021 Microsoft, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/ui/cocoa/window_buttons_view.h"

@implementation WindowButtonsView

- (id)initWithMargin:(const base::Optional<gfx::Point>&)margin {
  self = [super initWithFrame:NSZeroRect];
  [self setMargin:margin];

  mouse_inside_ = false;
  show_on_hover_ = false;

  NSButton* close_button =
      [NSWindow standardWindowButton:NSWindowCloseButton
                        forStyleMask:NSWindowStyleMaskTitled];
  [close_button setTag:1];
  NSButton* miniaturize_button =
      [NSWindow standardWindowButton:NSWindowMiniaturizeButton
                        forStyleMask:NSWindowStyleMaskTitled];
  [miniaturize_button setTag:2];
  NSButton* zoom_button =
      [NSWindow standardWindowButton:NSWindowZoomButton
                        forStyleMask:NSWindowStyleMaskTitled];
  [zoom_button setTag:3];

  CGFloat x = 0;
  const CGFloat space_between = 20;

  [close_button setFrameOrigin:NSMakePoint(x, 0)];
  x += space_between;
  [self addSubview:close_button];

  [miniaturize_button setFrameOrigin:NSMakePoint(x, 0)];
  x += space_between;
  [self addSubview:miniaturize_button];

  [zoom_button setFrameOrigin:NSMakePoint(x, 0)];
  x += space_between;
  [self addSubview:zoom_button];

  const auto last_button_frame = zoom_button.frame;
  [self setFrameSize:NSMakeSize(last_button_frame.origin.x +
                                    last_button_frame.size.width,
                                last_button_frame.size.height)];

  [self setNeedsDisplayForButtons];

  return self;
}

- (void)setMargin:(const base::Optional<gfx::Point>&)margin {
  margin_ = margin.value_or(gfx::Point(7, 3));
}

- (void)setShowOnHover:(BOOL)yes {
  show_on_hover_ = yes;
  [self setNeedsDisplayForButtons];
}

- (void)setNeedsDisplayForButtons {
  for (NSView* subview in self.subviews) {
    [subview setHidden:(show_on_hover_ && !mouse_inside_)];
    [subview setNeedsDisplay:YES];
  }
}

- (void)removeFromSuperview {
  [super removeFromSuperview];
  mouse_inside_ = NO;
}

- (void)viewDidMoveToWindow {
  // Stay in upper left corner.
  [self setAutoresizingMask:NSViewMaxXMargin | NSViewMinYMargin];
  [self setFrameOrigin:NSMakePoint(margin_.x(), self.window.frame.size.height -
                                                    self.frame.size.height -
                                                    margin_.y())];
}

- (BOOL)_mouseInGroup:(NSButton*)button {
  return mouse_inside_;
}

- (void)updateTrackingAreas {
  [super updateTrackingAreas];
  if (tracking_area_)
    [self removeTrackingArea:tracking_area_.get()];

  tracking_area_.reset([[NSTrackingArea alloc]
      initWithRect:NSZeroRect
           options:NSTrackingMouseEnteredAndExited | NSTrackingActiveAlways |
                   NSTrackingInVisibleRect
             owner:self
          userInfo:nil]);
  [self addTrackingArea:tracking_area_.get()];
}

- (void)mouseEntered:(NSEvent*)event {
  [super mouseEntered:event];
  mouse_inside_ = YES;
  [self setNeedsDisplayForButtons];
}

- (void)mouseExited:(NSEvent*)event {
  [super mouseExited:event];
  mouse_inside_ = NO;
  [self setNeedsDisplayForButtons];
}

@end
