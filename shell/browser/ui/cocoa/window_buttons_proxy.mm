// Copyright (c) 2021 Microsoft, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/ui/cocoa/window_buttons_proxy.h"

#include <cmath>
#include <memory>

#include "base/i18n/rtl.h"
#include "base/memory/raw_ptr.h"
#include "electron/mas.h"
#include "ui/views/controls/native/native_view_host.h"
#include "ui/views/view_class_properties.h"

#if !IS_MAS_BUILD()
@interface NSButton (FullscreenHover)
- (void)mouseEnteredOrExited;
@end
#endif

@interface WindowButtonsProxy () {
  raw_ptr<views::NativeViewHost> native_host_;
  __weak NSView* fullscreen_container_;
  NSArray<NSButton*>* fullscreen_buttons_;
  BOOL zoom_enabled_;
  CGFloat fullscreen_offset_;
  double reveal_;
}
- (CGFloat)buttonSpacing;
- (BOOL)mouseInside;
- (void)setMouseInside:(BOOL)inside;
- (void)zoomButtonAction:(id)sender;
- (void)redrawFullscreen;
@end

@implementation ButtonsAreaHoverView : NSView

- (id)initWithProxy:(WindowButtonsProxy*)proxy {
  if ((self = [super init])) {
    proxy_ = proxy;
  }
  return self;
}

// The normal hover overlay ignores clicks; factory controls remain native
// hit-testable children when this view presents fullscreen controls.
- (NSView*)hitTest:(NSPoint)aPoint {
  return self.subviews.count ? [super hitTest:aPoint] : nil;
}

#if !IS_MAS_BUILD()
// AppKit's factory window buttons query their parent's group hover state.
- (BOOL)_mouseInGroup:(NSButton*)button {
  return [proxy_ mouseInside];
}
#endif

- (void)updateTrackingAreas {
  [proxy_ updateTrackingAreas];
}

@end

@implementation WindowButtonsProxy

- (id)initWithWindow:(NSWindow*)window {
  window_ = window;
  show_on_hover_ = NO;
  mouse_inside_ = NO;

  // Remember the default margin.
  margin_ = default_margin_ = [self getCurrentMargin];
  visible_ = YES;
  // Custom height will be used if set larger than default
  height_ = 0;

  return self;
}

- (void)dealloc {
  if (hover_view_)
    [hover_view_ removeFromSuperview];
}

- (void)setVisible:(BOOL)visible {
  visible_ = visible;
  if (fullscreen_container_) {
    [self redraw];
    return;
  }
  NSView* titleBarContainer = [self titleBarContainer];
  if (!titleBarContainer)
    return;
  [titleBarContainer setHidden:!visible];
}

- (BOOL)isVisible {
  // The buttons' host view is hidden while the fullscreen bar is concealed.
  if (fullscreen_buttons_)
    return visible_;
  NSView* titleBarContainer = [self titleBarContainer];
  if (!titleBarContainer)
    return YES;
  return ![titleBarContainer isHidden];
}

- (void)setZoomButtonEnabled:(BOOL)enabled {
  zoom_enabled_ = enabled;
  if (fullscreen_buttons_)
    [self redraw];
}

- (BOOL)mouseInside {
  return mouse_inside_;
}

- (void)setMouseInside:(BOOL)inside {
  mouse_inside_ = inside;
  [self updateButtonsVisibility];
}

- (void)zoomButtonAction:(id)sender {
  // Native factory zoom actions depend on AppKit's managed-button hit state.
  if (NSApp.currentEvent.modifierFlags & NSEventModifierFlagOption)
    [window_ zoom:sender];
  else
    [window_ toggleFullScreen:sender];
}

- (CGFloat)buttonSpacing {
  return std::abs(
      NSMinX([self middleButton].frame) -
      NSMinX([window_ standardWindowButton:NSWindowCloseButton].frame));
}

- (void)setShowOnHover:(BOOL)yes {
  NSView* titleBarContainer = [self titleBarContainer];
  if (!titleBarContainer)
    return;
  show_on_hover_ = yes;
  // Put a transparent view above the window buttons so we can track mouse
  // events when mouse enter/leave the window buttons.
  if (show_on_hover_) {
    hover_view_ = [[ButtonsAreaHoverView alloc] initWithProxy:self];
    [hover_view_ setFrame:[self getButtonsBounds]];
    [titleBarContainer addSubview:hover_view_];
  } else {
    [hover_view_ removeFromSuperview];
    hover_view_ = nil;
  }
  [self updateButtonsVisibility];
}

- (void)setMargin:(const std::optional<gfx::Point>&)margin {
  has_custom_position_ = margin.has_value();
  if (margin)
    margin_ = *margin;
  else
    margin_ = default_margin_;
  [self redraw];
}

- (void)setHeight:(const float)height {
  height_ = height;
  [self redraw];
}

- (BOOL)useCustomHeight {
  NSView* left = [self leftButton];
  float button_height = NSHeight(left.frame);
  return height_ > button_height + 2 * default_margin_.y();
}

- (CGFloat)effectiveHeight {
  const CGFloat button_height = NSHeight([self leftButton].frame);
  return [self useCustomHeight] ? height_ : button_height + 2 * margin_.y();
}

- (void)beginFullscreenWithContainer:(NSView*)container
                            rootView:(views::View*)rootView
                      customTitlebar:(BOOL)customTitlebar {
  fullscreen_container_ = container;
  if (!customTitlebar)
    return;
  const CGFloat spacing = [self buttonSpacing];
  hover_view_ = [[ButtonsAreaHoverView alloc] initWithProxy:self];
  NSMutableArray<NSButton*>* buttons = [NSMutableArray array];
  for (NSWindowButton kind :
       {NSWindowCloseButton, NSWindowMiniaturizeButton, NSWindowZoomButton}) {
    NSButton* button = [NSWindow
        standardWindowButton:kind
                forStyleMask:window_.styleMask & ~NSWindowStyleMaskFullScreen];
    button.target = window_;
    if (kind == NSWindowZoomButton) {
      button.target = self;
      button.action = @selector(zoomButtonAction:);
    }
    [button setFrameOrigin:NSMakePoint(buttons.count * spacing, 0)];
    [hover_view_ addSubview:button];
    [buttons addObject:button];
  }
  fullscreen_buttons_ = buttons;
  auto host = std::make_unique<views::NativeViewHost>();
  host->SetProperty(views::kViewIgnoredByLayoutKey, true);
  native_host_ = rootView->AddChildView(std::move(host));
  native_host_->Attach(gfx::NativeView(hover_view_));
}

- (void)endFullscreen {
  if (fullscreen_buttons_) {
    if (mouse_inside_)
      [self setMouseInside:NO];
    native_host_->Detach();
    auto* parent = native_host_->parent();
    auto host = parent->RemoveChildViewT(native_host_.ExtractAsDangling());
    if (tracking_area_)
      [hover_view_ removeTrackingArea:tracking_area_];
    tracking_area_ = nil;
    hover_view_ = nil;
    fullscreen_buttons_ = nil;
  }
  fullscreen_container_ = nil;
}

- (CGFloat)layoutFullscreenAtOffset:(CGFloat)offset reveal:(double)reveal {
  fullscreen_offset_ = offset;
  reveal_ = reveal;
  [self redraw];
  return fullscreen_buttons_ && visible_
             ? NSWidth([self getButtonsContainerBounds]) * reveal
             : 0;
}

- (NSRect)getButtonsContainerBounds {
  if (fullscreen_buttons_) {
    return NSMakeRect(0, 0,
                      2 * margin_.x() + 2 * [self buttonSpacing] +
                          NSWidth(fullscreen_buttons_.firstObject.frame),
                      [self effectiveHeight]);
  }
  return NSInsetRect([self getButtonsBounds], -margin_.x(), -margin_.y());
}

- (void)redraw {
  if (fullscreen_container_) {
    [self redrawFullscreen];
    return;
  }
  NSView* titleBarContainer = [self titleBarContainer];
  if (!titleBarContainer)
    return;

  NSView* left = [self leftButton];
  NSView* middle = [self middleButton];
  NSView* right = [self rightButton];

  float button_width = NSWidth(left.frame);
  float button_height = NSHeight(left.frame);
  float padding = NSMinX(middle.frame) - NSMaxX(left.frame);
  float start;
  if (base::i18n::IsRTL())
    start =
        NSWidth(window_.frame) - 3 * button_width - 2 * padding - margin_.x();
  else
    start = margin_.x();

  NSRect cbounds = titleBarContainer.frame;
  cbounds.size.height = button_height + 2 * margin_.y();
  // Custom height must be larger than the button height to use
  if ([self useCustomHeight]) {
    cbounds.size.height = height_;
  }
  cbounds.origin.y = NSHeight(window_.frame) - NSHeight(cbounds);
  [titleBarContainer setFrame:cbounds];

  [left setFrameOrigin:NSMakePoint(start, [self getCurrentMargin].y())];
  start += button_width + padding;
  [middle setFrameOrigin:NSMakePoint(start, [self getCurrentMargin].y())];
  start += button_width + padding;
  [right setFrameOrigin:NSMakePoint(start, [self getCurrentMargin].y())];

  if (hover_view_)
    [hover_view_ setFrame:[self getButtonsBounds]];
}

- (void)redrawFullscreen {
  NSView* container = fullscreen_container_;
  if (!fullscreen_buttons_) {
    container.hidden = !visible_;
    return;
  }

  const NSSize size = fullscreen_buttons_.firstObject.frame.size;
  const CGFloat spacing = [self buttonSpacing];
  const CGFloat width = 2 * spacing + size.width;
  const CGFloat slide =
      NSWidth([self getButtonsContainerBounds]) * (1 - reveal_);
  const CGFloat host_x = margin_.x() - slide;
  const CGFloat top =
      fullscreen_offset_ + (has_custom_position_
                                ? margin_.y()
                                : ([self effectiveHeight] - size.height) / 2);
  // Views mirrors the host's leading edge; Cocoa still needs button order
  // mirrored.
  native_host_->SetBoundsRect(
      gfx::Rect(std::round(host_x), std::round(top), width, size.height));
  auto* parent = native_host_->parent();
  if (parent->children().back() != native_host_)
    parent->ReorderChildView(native_host_, parent->children().size() - 1);
  const BOOL visible = visible_ && reveal_ > 0;
  native_host_->SetVisible(visible);
  native_host_->DeprecatedLayoutImmediately();
  if (!tracking_area_ || !NSEqualRects(tracking_area_.rect, hover_view_.bounds))
    [self updateTrackingAreas];
  if (!visible && mouse_inside_)
    [self setMouseInside:NO];

  for (NSUInteger i = 0; i < fullscreen_buttons_.count; ++i) {
    NSButton* button = fullscreen_buttons_[i];
    CGFloat x = i * spacing;
    if (base::i18n::IsRTL())
      x = width - size.width - x;
    const NSRect frame = NSMakeRect(x, 0, size.width, size.height);
    if (!NSEqualRects(button.frame, frame))
      button.frame = frame;
    const BOOL enabled =
        i == 0 ? (window_.styleMask & NSWindowStyleMaskClosable) != 0
               : i == 2 && zoom_enabled_;
    if (button.enabled != enabled)
      button.enabled = enabled;
  }
  container.hidden = YES;
}

- (void)updateTrackingAreas {
  if (tracking_area_)
    [hover_view_ removeTrackingArea:tracking_area_];
  NSTrackingAreaOptions options =
      NSTrackingMouseEnteredAndExited | NSTrackingActiveAlways;
  if (!fullscreen_buttons_)
    options |= NSTrackingInVisibleRect;
  tracking_area_ = [[NSTrackingArea alloc]
      initWithRect:fullscreen_buttons_ ? hover_view_.bounds : NSZeroRect
           options:options
             owner:self
          userInfo:nil];
  [hover_view_ addTrackingArea:tracking_area_];
}

- (void)mouseEntered:(NSEvent*)event {
  [self setMouseInside:YES];
}

- (void)mouseExited:(NSEvent*)event {
  [self setMouseInside:NO];
}

- (void)updateButtonsVisibility {
  if (fullscreen_buttons_) {
    for (NSButton* button in fullscreen_buttons_) {
      button.needsDisplay = YES;
#if !IS_MAS_BUILD()
      // Since macOS 26 the native widgets also cache hover/modifier state.
      if (@available(macOS 26, *)) {
        if ([button respondsToSelector:@selector(mouseEnteredOrExited)])
          [button mouseEnteredOrExited];
      }
#endif
    }
    return;
  }
  // Show buttons when mouse hovers above them.
  BOOL hidden = show_on_hover_ && !mouse_inside_;
  // Always show buttons under fullscreen.
  if ([window_ styleMask] & NSWindowStyleMaskFullScreen)
    hidden = NO;
  for (NSWindowButton button_type :
       {NSWindowCloseButton, NSWindowMiniaturizeButton, NSWindowZoomButton}) {
    NSButton* button = [window_ standardWindowButton:button_type];
    if (!button)
      continue;
    [button setHidden:hidden];
    [button setNeedsDisplay:YES];
  }

  // On macOS 26, toggling the hidden state of the standard window buttons can
  // cause AppKit to re-layout the title bar container and reset its frame,
  // which loses the custom margin adjustments. Re-apply the calculated geometry
  // after visibility changes to keep the buttons at the specified margin
  // instead of snapping back to the default until the next manual resize.
  [self redraw];
}

// Return the bounds of all 3 buttons.
- (NSRect)getButtonsBounds {
  NSView* left = [self leftButton];
  NSView* right = [self rightButton];

  return NSMakeRect(NSMinX(left.frame), NSMinY(left.frame),
                    NSMaxX(right.frame) - NSMinX(left.frame),
                    NSHeight(left.frame));
}

// Compute margin from position of current buttons.
- (gfx::Point)getCurrentMargin {
  gfx::Point result;
  NSView* titleBarContainer = [self titleBarContainer];
  if (!titleBarContainer)
    return result;

  NSView* left = [self leftButton];
  NSView* right = [self rightButton];

  if (height_ != 0) {
    result.set_y((height_ - NSHeight(left.frame)) / 2);

    // Do not center buttons if height and button position specified
    if (margin_.y() != default_margin_.y())
      result.set_y(height_ - NSHeight(left.frame) - margin_.y());

  } else {
    result.set_y((NSHeight(titleBarContainer.frame) - NSHeight(left.frame)) /
                 2);
  }

  if (base::i18n::IsRTL())
    result.set_x(NSWidth(window_.frame) - NSMaxX(right.frame));
  else
    result.set_x(NSMinX(left.frame));
  return result;
}

// Receive the titlebar container, which might be nil if the window does not
// have the NSWindowStyleMaskTitled style.
- (NSView*)titleBarContainer {
  NSButton* left = [self leftButton];
  if ([fullscreen_buttons_ containsObject:left])
    return fullscreen_container_;
  return left.superview.superview;
}

// Receive the window buttons, note that the buttons might be removed and
// re-added on the fly so we should not cache them.
- (NSButton*)leftButton {
  if (base::i18n::IsRTL())
    return [window_ standardWindowButton:NSWindowZoomButton];
  else
    return [window_ standardWindowButton:NSWindowCloseButton];
}

- (NSButton*)middleButton {
  return [window_ standardWindowButton:NSWindowMiniaturizeButton];
}

- (NSButton*)rightButton {
  if (base::i18n::IsRTL())
    return [window_ standardWindowButton:NSWindowCloseButton];
  else
    return [window_ standardWindowButton:NSWindowZoomButton];
}

@end
