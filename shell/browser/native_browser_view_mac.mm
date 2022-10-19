// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/native_browser_view_mac.h"

#include "shell/browser/ui/inspectable_web_contents.h"
#include "shell/browser/ui/inspectable_web_contents_view.h"
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

  // Ensure draggable regions are properly updated to reflect new bounds.
  iwc_view->UpdateDraggableRegions(iwc_view->GetDraggableRegions());
}

gfx::Rect NativeBrowserViewMac::GetBounds() {
  auto* iwc_view = GetInspectableWebContentsView();
  if (!iwc_view)
    return gfx::Rect();
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

// static
NativeBrowserView* NativeBrowserView::Create(
    InspectableWebContents* inspectable_web_contents) {
  return new NativeBrowserViewMac(inspectable_web_contents);
}

}  // namespace electron
