// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/native_browser_view_mac.h"

#include "brightray/browser/inspectable_web_contents_view.h"
#include "skia/ext/skia_utils_mac.h"
#include "ui/gfx/geometry/rect.h"

namespace atom {

NativeBrowserViewMac::NativeBrowserViewMac(
    brightray::InspectableWebContentsView* web_contents_view)
    : NativeBrowserView(web_contents_view) {}

NativeBrowserViewMac::~NativeBrowserViewMac() {}

void NativeBrowserViewMac::SetBounds(const gfx::Rect& bounds) {
  auto* view = GetInspectableWebContentsView()->GetNativeView();
  auto* superview = view.superview;
  const auto superview_height = superview ? superview.frame.size.height : 0;
  view.frame =
      NSMakeRect(bounds.x(), superview_height - bounds.y() - bounds.height(),
                 bounds.width(), bounds.height());
}

void NativeBrowserViewMac::SetBackgroundColor(SkColor color) {
  auto* view = GetInspectableWebContentsView()->GetNativeView();
  view.wantsLayer = YES;
  view.layer.backgroundColor = skia::CGColorCreateFromSkColor(color);
}

// static
NativeBrowserView* NativeBrowserView::Create(
    brightray::InspectableWebContentsView* web_contents_view) {
  return new NativeBrowserViewMac(web_contents_view);
}

}  // namespace atom
