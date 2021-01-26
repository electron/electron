// Copyright (c) 2021 Microsoft, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/ui/cocoa/window_buttons_view.h"

#include "base/i18n/rtl.h"
#include "base/logging.h"
#include "base/stl_util.h"
#include "ui/gfx/mac/coordinate_conversion.h"

namespace {

const CGFloat kButtonPadding = 20.;

const NSWindowButton kButtonTypes[] = {
    NSWindowCloseButton,
    NSWindowMiniaturizeButton,
    NSWindowZoomButton,
};

}  // namespace

@implementation WindowButtonsView

- (id)initWithMargin:(const base::Optional<gfx::Point>&)margin {
  self = [super initWithFrame:NSZeroRect];
  [self setMargin:margin];

  mouse_inside_ = false;
  show_on_hover_ = false;
  is_rtl_ = base::i18n::IsRTL();

  for (size_t i = 0; i < base::size(kButtonTypes); ++i) {
    NSButton* button = [NSWindow standardWindowButton:kButtonTypes[i]
                                         forStyleMask:NSWindowStyleMaskTitled];
    [button setTag:i];
    int left_index = is_rtl_ ? base::size(kButtonTypes) - i - 1 : i;
    [button setFrameOrigin:NSMakePoint(left_index * kButtonPadding, 0)];
    [self addSubview:button];
  }

  NSView* last_button =
      is_rtl_ ? [[self subviews] firstObject] : [[self subviews] lastObject];
  [self setFrameSize:NSMakeSize(last_button.frame.origin.x +
                                    last_button.frame.size.width,
                                last_button.frame.size.height)];
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
  CGFloat y =
      self.superview.frame.size.height - self.frame.size.height - margin_.y();
  if (is_rtl_) {
    CGFloat x =
        self.superview.frame.size.width - self.frame.size.width - margin_.x();
    [self setAutoresizingMask:NSViewMinXMargin | NSViewMinYMargin];
    [self setFrameOrigin:NSMakePoint(x, y)];
  } else {
    [self setAutoresizingMask:NSViewMaxXMargin | NSViewMinYMargin];
    [self setFrameOrigin:NSMakePoint(margin_.x(), y)];
  }
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
