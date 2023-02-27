// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/native_browser_view_mac.h"

#include <string>

#include "shell/browser/ui/inspectable_web_contents.h"
#include "shell/browser/ui/inspectable_web_contents_view.h"
#include "shell/browser/ui/inspectable_web_contents_view_delegate.h"
#include "skia/ext/skia_utils_mac.h"
#include "ui/gfx/geometry/rect.h"

// Match view::Views behavior where the view sticks to the top-left origin.
const NSAutoresizingMaskOptions kDefaultAutoResizingMask =
    NSViewMaxXMargin | NSViewMinYMargin;

namespace electron {

NativeBrowserViewMac::NativeBrowserViewMac(
    InspectableWebContents* inspectable_web_contents)
    : NativeBrowserView(inspectable_web_contents) {
  auto* iwc_view = GetInspectableWebContentsView();
  if (!iwc_view)
    return;
  auto* view = iwc_view->GetNativeView().GetNativeNSView();
  view.autoresizingMask = kDefaultAutoResizingMask;
}

NativeBrowserViewMac::~NativeBrowserViewMac() = default;

void NativeBrowserViewMac::SetAutoResizeFlags(uint8_t flags) {
  NSAutoresizingMaskOptions autoresizing_mask = kDefaultAutoResizingMask;
  if (flags & kAutoResizeWidth) {
    autoresizing_mask |= NSViewWidthSizable;
  }
  if (flags & kAutoResizeHeight) {
    autoresizing_mask |= NSViewHeightSizable;
  }
  if (flags & kAutoResizeHorizontal) {
    autoresizing_mask |=
        NSViewMaxXMargin | NSViewMinXMargin | NSViewWidthSizable;
  }
  if (flags & kAutoResizeVertical) {
    autoresizing_mask |=
        NSViewMaxYMargin | NSViewMinYMargin | NSViewHeightSizable;
  }

  auto* iwc_view = GetInspectableWebContentsView();
  if (!iwc_view)
    return;
  auto* view = iwc_view->GetNativeView().GetNativeNSView();
  view.autoresizingMask = autoresizing_mask;
}

void NativeBrowserViewMac::SetBounds(const gfx::Rect& bounds) {
  auto* iwc_view = GetInspectableWebContentsView();
  if (!iwc_view)
    return;

  if (popover_) {
    [popover_ setContentSize:NSMakeSize(bounds.width(), bounds.height())];
    return;
  }

  auto* view = iwc_view->GetNativeView().GetNativeNSView();
  auto* superview = view.superview;
  const auto superview_height = superview ? superview.frame.size.height : 0;

  // We need to use the content rect to calculate the titlebar height if the
  // superview is an framed NSWindow, otherwise it will be offset incorrectly by
  // the height of the titlebar.
  auto titlebar_height = 0;
  if (auto* win = [superview window]) {
    const auto content_rect_height =
        [win contentRectForFrameRect:superview.frame].size.height;
    titlebar_height = superview_height - content_rect_height;
  }

  auto new_height =
      superview_height - bounds.y() - bounds.height() + titlebar_height;
  view.frame =
      NSMakeRect(bounds.x(), new_height, bounds.width(), bounds.height());
}

gfx::Rect NativeBrowserViewMac::GetBounds() {
  auto* iwc_view = GetInspectableWebContentsView();
  if (!iwc_view)
    return gfx::Rect();

  if (popover_) {
    return gfx::Rect(0, 0, [popover_ contentSize].width,
                     [popover_ contentSize].height);
  }

  NSView* view = iwc_view->GetNativeView().GetNativeNSView();
  auto* superview = view.superview;
  const int superview_height = superview ? superview.frame.size.height : 0;

  // We need to use the content rect to calculate the titlebar height if the
  // superview is an framed NSWindow, otherwise it will be offset incorrectly by
  // the height of the titlebar.
  auto titlebar_height = 0;
  if (auto* win = [superview window]) {
    const auto content_rect_height =
        [win contentRectForFrameRect:superview.frame].size.height;
    titlebar_height = superview_height - content_rect_height;
  }

  auto new_height = superview_height - view.frame.origin.y -
                    view.frame.size.height + titlebar_height;
  return gfx::Rect(view.frame.origin.x, new_height, view.frame.size.width,
                   view.frame.size.height);
}

void NativeBrowserViewMac::SetBackgroundColor(SkColor color) {
  auto* iwc_view = GetInspectableWebContentsView();
  if (!iwc_view)
    return;
  auto* view = iwc_view->GetNativeView().GetNativeNSView();
  view.wantsLayer = YES;
  view.layer.backgroundColor = skia::CGColorCreateFromSkColor(color);
}

void NativeBrowserViewMac::ShowPopoverWindow(NativeWindow* positioning_window,
                                             const gfx::Rect& positioning_rect,
                                             const gfx::Size& size,
                                             const std::string& preferred_edge,
                                             const std::string& behavior,
                                             bool animate) {
  if (!popover_) {
    auto* iwc_view = GetInspectableWebContentsView();
    if (!iwc_view)
      return;
    NSView* view = iwc_view->GetNativeView().GetNativeNSView();

    NSViewController* view_controller =
        [[[NSViewController alloc] init] autorelease];
    NSPopover* popover = [[NSPopover alloc] init];

    [popover setContentViewController:view_controller];

    [view setWantsLayer:YES];
    [popover.contentViewController setView:view];

    [popover setContentSize:NSMakeSize(size.width(), size.height())];

    id observer = [[NSNotificationCenter defaultCenter]
        addObserverForName:NSPopoverDidCloseNotification
                    object:popover
                     queue:nil
                usingBlock:^(NSNotification* notification) {
                  PopoverWindowClosed();
                }];

    popover_closed_observer_.reset(observer, base::scoped_policy::RETAIN);
    popover_.reset(popover);
  }

  NSPopoverBehavior popover_behavior = NSPopoverBehaviorApplicationDefined;
  if (behavior == "transient") {
    popover_behavior = NSPopoverBehaviorTransient;
  }

  NSRectEdge popover_edge = NSMaxXEdge;
  if (preferred_edge == "max-y-edge") {
    popover_edge = NSMaxYEdge;
  } else if (preferred_edge == "min-x-edge") {
    popover_edge = NSMinXEdge;
  } else if (preferred_edge == "min-y-edge") {
    popover_edge = NSMinYEdge;
  }

  [popover_ setBehavior:popover_behavior];
  [popover_ setAnimates:animate];

  NSWindow* positioning_ns_window =
      positioning_window->GetNativeWindow().GetNativeNSWindow();
  NSRect positioning_ns_rect =
      NSMakeRect(positioning_rect.x(), positioning_rect.y(),
                 positioning_rect.width(), positioning_rect.height());

  [popover_ showRelativeToRect:positioning_ns_rect
                        ofView:positioning_ns_window.contentView
                 preferredEdge:popover_edge];
}

void NativeBrowserViewMac::PopoverWindowClosed() {
  auto* iwc_view = GetInspectableWebContentsView();
  if (iwc_view) {
    iwc_view->GetDelegate()->PopoverClosed();
  }

  [[NSNotificationCenter defaultCenter]
      removeObserver:popover_closed_observer_.get()];

  popover_closed_observer_.reset();
  popover_.reset();
}

void NativeBrowserViewMac::ClosePopoverWindow() {
  if (popover_) {
    [popover_ close];
  }
}

// static
NativeBrowserView* NativeBrowserView::Create(
    InspectableWebContents* inspectable_web_contents) {
  return new NativeBrowserViewMac(inspectable_web_contents);
}

}  // namespace electron
