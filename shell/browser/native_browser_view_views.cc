// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/native_browser_view_views.h"

#include <vector>

#include "shell/browser/ui/drag_util.h"
#include "shell/browser/ui/views/inspectable_web_contents_view_views.h"
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

void NativeBrowserViewViews::UpdateDraggableRegions(
    const std::vector<mojom::DraggableRegionPtr>& regions) {
  // We need to snap the regions to the bounds of the current BrowserView.
  // For example, if an attached BrowserView is draggable but its bounds are
  // { x: 200,  y: 100, width: 300, height: 300 }
  // then we need to add 200 to the x-value and 100 to the
  // y-value of each of the passed regions or it will be incorrectly
  // assumed that the regions begin in the top left corner as they
  // would for the main client window.
  auto const offset = GetBounds().OffsetFromOrigin();
  auto snapped_regions = mojo::Clone(regions);
  for (auto& snapped_region : snapped_regions) {
    snapped_region->bounds.Offset(offset);
  }

  draggable_region_ = DraggableRegionsToSkRegion(snapped_regions);
}

void NativeBrowserViewViews::SetAutoResizeProportions(
    const gfx::Size& window_size) {
  if ((auto_resize_flags_ & AutoResizeFlags::kAutoResizeHorizontal) &&
      !auto_horizontal_proportion_set_) {
    InspectableWebContentsView* iwc_view = GetInspectableWebContentsView();
    if (!iwc_view)
      return;
    auto* view = iwc_view->GetView();
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
    InspectableWebContentsView* iwc_view = GetInspectableWebContentsView();
    if (!iwc_view)
      return;
    auto* view = iwc_view->GetView();
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
  InspectableWebContentsView* iwc_view = GetInspectableWebContentsView();
  if (!iwc_view)
    return;
  auto* view = iwc_view->GetView();
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
  InspectableWebContentsView* iwc_view = GetInspectableWebContentsView();
  if (!iwc_view)
    return;
  auto* view = iwc_view->GetView();
  view->SetBoundsRect(bounds);
  ResetAutoResizeProportions();
}

gfx::Rect NativeBrowserViewViews::GetBounds() {
  InspectableWebContentsView* iwc_view = GetInspectableWebContentsView();
  if (!iwc_view)
    return gfx::Rect();
  return iwc_view->GetView()->bounds();
}

void NativeBrowserViewViews::RenderViewReady() {
  InspectableWebContentsView* iwc_view = GetInspectableWebContentsView();
  if (iwc_view)
    iwc_view->GetView()->Layout();
}

void NativeBrowserViewViews::SetBackgroundColor(SkColor color) {
  InspectableWebContentsView* iwc_view = GetInspectableWebContentsView();
  if (!iwc_view)
    return;
  auto* view = iwc_view->GetView();
  view->SetBackground(views::CreateSolidBackground(color));
  view->SchedulePaint();
}

// static
NativeBrowserView* NativeBrowserView::Create(
    InspectableWebContents* inspectable_web_contents) {
  return new NativeBrowserViewViews(inspectable_web_contents);
}

}  // namespace electron
