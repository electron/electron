// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/native_browser_view_views.h"

#include "atom/browser/ui/inspectable_web_contents_view.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/views/background.h"
#include "ui/views/view.h"

namespace atom {

NativeBrowserViewViews::NativeBrowserViewViews(
    InspectableWebContents* inspectable_web_contents)
    : NativeBrowserView(inspectable_web_contents) {}

NativeBrowserViewViews::~NativeBrowserViewViews() {}

void NativeBrowserViewViews::SetAutoResizeFlags(uint8_t flags) {
  auto_resize_flags_ = flags;
}

void NativeBrowserViewViews::SetBounds(const gfx::Rect& bounds) {
  auto* view = GetInspectableWebContentsView()->GetView();
  view->SetBoundsRect(bounds);
}

void NativeBrowserViewViews::SetBackgroundColor(SkColor color) {
  auto* view = GetInspectableWebContentsView()->GetView();
  view->SetBackground(views::CreateSolidBackground(color));
  view->SchedulePaint();
}

// static
NativeBrowserView* NativeBrowserView::Create(
    InspectableWebContents* inspectable_web_contents) {
  return new NativeBrowserViewViews(inspectable_web_contents);
}

}  // namespace atom
