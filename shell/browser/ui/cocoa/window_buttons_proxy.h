// Copyright (c) 2021 Microsoft, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_UI_COCOA_WINDOW_BUTTONS_PROXY_H_
#define ELECTRON_SHELL_BROWSER_UI_COCOA_WINDOW_BUTTONS_PROXY_H_

#import <Cocoa/Cocoa.h>

#include <optional>

#include "ui/gfx/geometry/point.h"

namespace views {
class View;
}

@class WindowButtonsProxy;

// A helper view that floats above the window buttons.
@interface ButtonsAreaHoverView : NSView {
 @private
  __weak WindowButtonsProxy* proxy_;
}
- (id)initWithProxy:(WindowButtonsProxy*)proxy;
@end

// Manipulating the window buttons.
@interface WindowButtonsProxy : NSObject {
 @private
  NSWindow* window_;

  // Current left-top margin of buttons.
  gfx::Point margin_;
  BOOL has_custom_position_;
  // The default left-top margin.
  gfx::Point default_margin_;
  // Current height of the title bar container.
  float height_;
  // Temporary transition/miniaturize visibility, separate from caller intent.
  BOOL visible_;

  // Track mouse moves above window buttons.
  BOOL show_on_hover_;
  BOOL mouse_inside_;
  NSTrackingArea* __strong tracking_area_;
  ButtonsAreaHoverView* __strong hover_view_;
}

- (id)initWithWindow:(NSWindow*)window;

- (void)setVisible:(BOOL)visible;
- (BOOL)isVisible;
- (void)setZoomButtonEnabled:(BOOL)enabled;

// Only show window buttons when mouse moves above them.
- (void)setShowOnHover:(BOOL)yes;

// Set left-top margin of the window buttons..
- (void)setMargin:(const std::optional<gfx::Point>&)margin;

// Set height of button container
- (void)setHeight:(const float)height;
- (BOOL)useCustomHeight;
- (CGFloat)effectiveHeight;

- (NSView*)titleBarContainer;
- (void)beginFullscreenWithContainer:(NSView*)container
                            rootView:(views::View*)rootView
                      customTitlebar:(BOOL)customTitlebar;
- (void)endFullscreen;
- (CGFloat)layoutFullscreenAtOffset:(CGFloat)offset reveal:(double)reveal;

// Return the bounds of all 3 buttons, with margin on all sides.
- (NSRect)getButtonsContainerBounds;

- (void)redraw;
- (void)updateTrackingAreas;
@end

#endif  // ELECTRON_SHELL_BROWSER_UI_COCOA_WINDOW_BUTTONS_PROXY_H_
