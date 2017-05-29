// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <algorithm>
#include <utility>
#include <vector>

#include "atom/browser/osr/osr_render_widget_host_view.h"
#include "atom/browser/osr/osr_view_proxy.h"
#include "atom/browser/ui/autofill_popup.h"
#include "atom/common/api/api_messages.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/vector2d.h"
#include "ui/gfx/text_utils.h"

namespace atom {

namespace {

std::pair<int, int> CalculatePopupXAndWidth(
    const display::Display& left_display,
    const display::Display& right_display,
    int popup_required_width,
    const gfx::Rect& element_bounds,
    bool is_rtl) {
  int leftmost_display_x = left_display.bounds().x();
  int rightmost_display_x =
      right_display.GetSizeInPixel().width() + right_display.bounds().x();

  // Calculate the start coordinates for the popup if it is growing right or
  // the end position if it is growing to the left, capped to screen space.
  int right_growth_start = std::max(
      leftmost_display_x, std::min(rightmost_display_x, element_bounds.x()));
  int left_growth_end =
      std::max(leftmost_display_x,
               std::min(rightmost_display_x, element_bounds.right()));

  int right_available = rightmost_display_x - right_growth_start;
  int left_available = left_growth_end - leftmost_display_x;

  int popup_width =
      std::min(popup_required_width, std::max(right_available, left_available));

  std::pair<int, int> grow_right(right_growth_start, popup_width);
  std::pair<int, int> grow_left(left_growth_end - popup_width, popup_width);

  // Prefer to grow towards the end (right for LTR, left for RTL). But if there
  // is not enough space available in the desired direction and more space in
  // the other direction, reverse it.
  if (is_rtl) {
    return left_available >= popup_width || left_available >= right_available
               ? grow_left
               : grow_right;
  }
  return right_available >= popup_width || right_available >= left_available
             ? grow_right
             : grow_left;
}

std::pair<int, int> CalculatePopupYAndHeight(
    const display::Display& top_display,
    const display::Display& bottom_display,
    int popup_required_height,
    const gfx::Rect& element_bounds) {
  int topmost_display_y = top_display.bounds().y();
  int bottommost_display_y =
      bottom_display.GetSizeInPixel().height() + bottom_display.bounds().y();

  // Calculate the start coordinates for the popup if it is growing down or
  // the end position if it is growing up, capped to screen space.
  int top_growth_end = std::max(
      topmost_display_y, std::min(bottommost_display_y, element_bounds.y()));
  int bottom_growth_start =
      std::max(topmost_display_y,
               std::min(bottommost_display_y, element_bounds.bottom()));

  int top_available = bottom_growth_start - topmost_display_y;
  int bottom_available = bottommost_display_y - top_growth_end;

  // TODO(csharp): Restrict the popup height to what is available.
  if (bottom_available >= popup_required_height ||
      bottom_available >= top_available) {
    // The popup can appear below the field.
    return std::make_pair(bottom_growth_start, popup_required_height);
  } else {
    // The popup must appear above the field.
    return std::make_pair(top_growth_end - popup_required_height,
                          popup_required_height);
  }
}

display::Display GetDisplayNearestPoint(
    const gfx::Point& point,
    gfx::NativeView container_view) {
  return display::Screen::GetScreen()->GetDisplayNearestPoint(point);
}

}  // namespace

AutofillPopup::AutofillPopup(gfx::NativeView container_view)
    : container_view_(container_view), view_(nullptr) {
  bold_font_list_ =
    gfx::FontList().DeriveWithWeight(gfx::Font::Weight::BOLD);
  smaller_font_list_ =
    gfx::FontList().DeriveWithSizeDelta(kSmallerFontSizeDelta);
}

AutofillPopup::~AutofillPopup() {
  Hide();
}

void AutofillPopup::CreateView(
    content::RenderFrameHost* frame_host,
    bool offscreen,
    views::Widget* parent_widget,
    const gfx::RectF& r) {
  frame_host_ = frame_host;
  gfx::Rect lb(std::floor(r.x()), std::floor(r.y() + r.height()),
               std::floor(r.width()), std::floor(r.height()));
  gfx::Point menu_position(lb.origin());
  popup_bounds_in_view_ = lb;
  views::View::ConvertPointToScreen(
    parent_widget->GetContentsView(), &menu_position);
  popup_bounds_ = gfx::Rect(menu_position, lb.size());
  element_bounds_ = popup_bounds_;

  Hide();
  view_ = new AutofillPopupView(this, parent_widget);
  view_->Show();

  if (offscreen) {
    auto* osr_rwhv = static_cast<OffScreenRenderWidgetHostView*>(
        frame_host_->GetView());
    view_->view_proxy_.reset(new OffscreenViewProxy(view_));
    osr_rwhv->AddViewProxy(view_->view_proxy_.get());
  }
}

void AutofillPopup::Hide() {
  if (view_) {
    view_->Hide();
    view_ = nullptr;
  }
}

void AutofillPopup::SetItems(const std::vector<base::string16>& values,
                            const std::vector<base::string16>& labels) {
  values_ = values;
  labels_ = labels;
  UpdatePopupBounds();
  if (view_) {
    view_->OnSuggestionsChanged();
  }
}

void AutofillPopup::AcceptSuggestion(int index) {
  frame_host_->Send(new AtomAutofillFrameMsg_AcceptSuggestion(
    frame_host_->GetRoutingID(), GetValueAt(index)));
}

void AutofillPopup::UpdatePopupBounds() {
  int desired_width = GetDesiredPopupWidth();
  int desired_height = GetDesiredPopupHeight();
  bool is_rtl = false;

  gfx::Point top_left_corner_of_popup =
      element_bounds_.origin() +
      gfx::Vector2d(element_bounds_.width() - desired_width, -desired_height);

  // This is the bottom right point of the popup if the popup is below the
  // element and grows to the right (since the is the lowest and furthest right
  // the popup could go).
  gfx::Point bottom_right_corner_of_popup =
      element_bounds_.origin() +
      gfx::Vector2d(desired_width, element_bounds_.height() + desired_height);

  display::Display top_left_display =
      GetDisplayNearestPoint(top_left_corner_of_popup, container_view_);
  display::Display bottom_right_display =
      GetDisplayNearestPoint(bottom_right_corner_of_popup, container_view_);

  std::pair<int, int> popup_x_and_width =
      CalculatePopupXAndWidth(top_left_display, bottom_right_display,
                              desired_width, element_bounds_, is_rtl);
  std::pair<int, int> popup_y_and_height = CalculatePopupYAndHeight(
      top_left_display, bottom_right_display, desired_height, element_bounds_);

  popup_bounds_ = gfx::Rect(popup_x_and_width.first, popup_y_and_height.first,
      popup_x_and_width.second, popup_y_and_height.second);
  popup_bounds_in_view_ = gfx::Rect(popup_bounds_in_view_.origin(),
      gfx::Size(popup_x_and_width.second, popup_y_and_height.second));
}

int AutofillPopup::GetDesiredPopupHeight() {
  return 2 * kPopupBorderThickness + values_.size() * kRowHeight;
}

int AutofillPopup::GetDesiredPopupWidth() {
  int popup_width = element_bounds_.width();

  for (size_t i = 0; i < values_.size(); ++i) {
    int row_size = kEndPadding + 2 * kPopupBorderThickness +
      gfx::GetStringWidth(GetValueAt(i), GetValueFontListForRow(i)) +
      gfx::GetStringWidth(GetLabelAt(i), GetLabelFontListForRow(i));
    if (GetLabelAt(i).length() > 0)
      row_size += kNamePadding + kEndPadding;

    popup_width = std::max(popup_width, row_size);
  }

  return popup_width;
}

gfx::Rect AutofillPopup::GetRowBounds(int index) {
  int top = kPopupBorderThickness + index * kRowHeight;

  return gfx::Rect(kPopupBorderThickness, top,
                   popup_bounds_.width() - 2 * kPopupBorderThickness,
                   kRowHeight);
}

const gfx::FontList& AutofillPopup::GetValueFontListForRow(int index) const {
  return bold_font_list_;
}

const gfx::FontList& AutofillPopup::GetLabelFontListForRow(int index) const {
  return smaller_font_list_;
}

ui::NativeTheme::ColorId AutofillPopup::GetBackgroundColorIDForRow(
    int index) const {
  return (view_ && index == view_->GetSelectedLine())
      ? ui::NativeTheme::kColorId_ResultsTableHoveredBackground
      : ui::NativeTheme::kColorId_ResultsTableNormalBackground;
}

int AutofillPopup::GetLineCount() {
  return values_.size();
}

base::string16 AutofillPopup::GetValueAt(int i) {
  return values_.at(i);
}

base::string16 AutofillPopup::GetLabelAt(int i) {
  return labels_.at(i);
}

int AutofillPopup::LineFromY(int y) const {
  int current_height = kPopupBorderThickness;

  for (size_t i = 0; i < values_.size(); ++i) {
    current_height += kRowHeight;

    if (y <= current_height)
      return i;
  }

  return values_.size() - 1;
}

}  // namespace atom
