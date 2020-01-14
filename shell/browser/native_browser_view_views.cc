// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/native_browser_view_views.h"

#include "shell/browser/ui/inspectable_web_contents_view.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/views/background.h"
#include "ui/views/view.h"

namespace electron {

NativeBrowserViewViews::NativeBrowserViewViews(
    InspectableWebContents* inspectable_web_contents)
    : NativeBrowserView(inspectable_web_contents) {}

NativeBrowserViewViews::~NativeBrowserViewViews() = default;

void NativeBrowserViewViews::SetAutoResizeFlags(uint8_t flags) {
  auto_resize_flags_ = flags;
  ResetAutoResizeProportions();
}

void NativeBrowserViewViews::SetAutoResizeProportions(
    const gfx::Size& window_size) {
  if ((auto_resize_flags_ & AutoResizeFlags::kAutoResizeHorizontal) &&
      !auto_horizontal_proportion_set_) {
    auto* view = GetInspectableWebContentsView()->GetView();
    auto view_bounds = view->bounds();
    auto_horizontal_proportion_width_ =
        static_cast<float>(window_size.width()) /
        static_cast<float>(view_bounds.width());
    auto_horizontal_proportion_left_ = static_cast<float>(window_size.width()) /
                                       static_cast<float>(view_bounds.x());
    auto_horizontal_proportion_set_ = true;
  }
  if ((auto_resize_flags_ & AutoResizeFlags::kAutoResizeVertical) &&
      !auto_vertical_proportion_set_) {
    auto* view = GetInspectableWebContentsView()->GetView();
    auto view_bounds = view->bounds();
    auto_vertical_proportion_height_ =
        static_cast<float>(window_size.height()) /
        static_cast<float>(view_bounds.height());
    auto_vertical_proportion_top_ = static_cast<float>(window_size.height()) /
                                    static_cast<float>(view_bounds.y());
    auto_vertical_proportion_set_ = true;
  }
}

void NativeBrowserViewViews::AutoResize(const gfx::Rect& new_window,
                                        int width_delta,
                                        int height_delta) {
  auto* view = GetInspectableWebContentsView()->GetView();
  const auto flags = GetAutoResizeFlags();
  if (!(flags & kAutoResizeWidth)) {
    width_delta = 0;
  }
  if (!(flags & kAutoResizeHeight)) {
    height_delta = 0;
  }
  if (height_delta || width_delta) {
    auto new_view_size = view->size();
    new_view_size.set_width(new_view_size.width() + width_delta);
    new_view_size.set_height(new_view_size.height() + height_delta);
    view->SetSize(new_view_size);
  }
  auto new_view_bounds = view->bounds();
  if (flags & kAutoResizeHorizontal) {
    new_view_bounds.set_width(new_window.width() /
                              auto_horizontal_proportion_width_);
    new_view_bounds.set_x(new_window.width() /
                          auto_horizontal_proportion_left_);
  }
  if (flags & kAutoResizeVertical) {
    new_view_bounds.set_height(new_window.height() /
                               auto_vertical_proportion_height_);
    new_view_bounds.set_y(new_window.height() / auto_vertical_proportion_top_);
  }
  if ((flags & kAutoResizeHorizontal) || (flags & kAutoResizeVertical)) {
    view->SetBoundsRect(new_view_bounds);
  }
}

void NativeBrowserViewViews::ResetAutoResizeProportions() {
  if (auto_resize_flags_ & AutoResizeFlags::kAutoResizeHorizontal) {
    auto_horizontal_proportion_set_ = false;
  }
  if (auto_resize_flags_ & AutoResizeFlags::kAutoResizeVertical) {
    auto_vertical_proportion_set_ = false;
  }
}

void NativeBrowserViewViews::SetBounds(const gfx::Rect& bounds) {
  auto* view = GetInspectableWebContentsView()->GetView();
  view->SetBoundsRect(bounds);
  ResetAutoResizeProportions();
}

gfx::Rect NativeBrowserViewViews::GetBounds() {
  return GetInspectableWebContentsView()->GetView()->bounds();
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

}  // namespace electron
