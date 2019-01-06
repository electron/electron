// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <algorithm>
#include <utility>
#include <vector>

#include "atom/browser/native_window_views.h"
#include "atom/browser/ui/autofill_popup.h"
#include "atom/common/api/api_messages.h"
#include "base/i18n/rtl.h"
#include "electron/buildflags/buildflags.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/rect_conversions.h"
#include "ui/gfx/geometry/vector2d.h"
#include "ui/gfx/text_utils.h"

#if BUILDFLAG(ENABLE_OSR)
#include "atom/browser/osr/osr_render_widget_host_view.h"
#include "atom/browser/osr/osr_view_proxy.h"
#endif

namespace atom {

namespace {

struct WidthCalculationResults {
  int right_growth_start;
  int left_growth_end;
  int right_available;
  int left_available;
  int max_popup_width;
};

WidthCalculationResults CalculateWidthValues(int leftmost_available_x,
                                             int rightmost_available_x,
                                             const gfx::Rect& element_bounds) {
  WidthCalculationResults result;
  result.right_growth_start =
      std::max(leftmost_available_x,
               std::min(rightmost_available_x, element_bounds.x()));
  result.left_growth_end =
      std::max(leftmost_available_x,
               std::min(rightmost_available_x, element_bounds.right()));

  result.right_available = rightmost_available_x - result.right_growth_start;
  result.left_available = result.left_growth_end - leftmost_available_x;

  result.max_popup_width =
      std::max(result.right_available, result.left_available);
  return result;
}

void CalculatePopupXAndWidth(int leftmost_available_x,
                             int rightmost_available_x,
                             int popup_required_width,
                             const gfx::Rect& element_bounds,
                             bool is_rtl,
                             gfx::Rect* popup_bounds) {
  WidthCalculationResults result = CalculateWidthValues(
      leftmost_available_x, rightmost_available_x, element_bounds);

  int popup_width = std::min(popup_required_width, result.max_popup_width);

  bool grow_left = false;
  if (is_rtl) {
    grow_left = result.left_available >= popup_width ||
                result.left_available >= result.right_available;
  } else {
    grow_left = result.right_available < popup_width &&
                result.right_available < result.left_available;
  }

  popup_bounds->set_width(popup_width);
  popup_bounds->set_x(grow_left ? result.left_growth_end - popup_width
                                : result.right_growth_start);
}

void CalculatePopupYAndHeight(int topmost_available_y,
                              int bottommost_available_y,
                              int popup_required_height,
                              const gfx::Rect& element_bounds,
                              gfx::Rect* popup_bounds) {
  int top_growth_end =
      std::max(topmost_available_y,
               std::min(bottommost_available_y, element_bounds.y()));
  int bottom_growth_start =
      std::max(topmost_available_y,
               std::min(bottommost_available_y, element_bounds.bottom()));

  int top_available = top_growth_end - topmost_available_y;
  int bottom_available = bottommost_available_y - bottom_growth_start;

  if (bottom_available >= popup_required_height ||
      bottom_available >= top_available) {
    popup_bounds->set_height(std::min(bottom_available, popup_required_height));
    popup_bounds->set_y(bottom_growth_start);
  } else {
    popup_bounds->set_height(std::min(top_available, popup_required_height));
    popup_bounds->set_y(top_growth_end - popup_bounds->height());
  }
}

gfx::Rect CalculatePopupBounds(int desired_width,
                               int desired_height,
                               const gfx::Rect& element_bounds,
                               gfx::Rect window_bounds,
                               bool is_rtl) {
  gfx::Rect popup_bounds;
  CalculatePopupXAndWidth(window_bounds.x(),
                          window_bounds.x() + window_bounds.width(),
                          desired_width, element_bounds, is_rtl, &popup_bounds);
  CalculatePopupYAndHeight(window_bounds.y(),
                           window_bounds.y() + window_bounds.height(),
                           desired_height, element_bounds, &popup_bounds);

  return popup_bounds;
}

}  // namespace

AutofillPopup::AutofillPopup() {
  bold_font_list_ = gfx::FontList().DeriveWithWeight(gfx::Font::Weight::BOLD);
  smaller_font_list_ =
      gfx::FontList().DeriveWithSizeDelta(kSmallerFontSizeDelta);
}

AutofillPopup::~AutofillPopup() {
  Hide();
}

void AutofillPopup::CreateView(content::RenderFrameHost* frame_host,
                               content::RenderFrameHost* embedder_frame_host,
                               bool offscreen,
                               views::View* parent,
                               const gfx::RectF& r) {
  Hide();

  frame_host_ = frame_host;
  element_bounds_ = gfx::ToEnclosedRect(r);

  gfx::Vector2d height_offset(0, element_bounds_.height());
  gfx::Point menu_position(element_bounds_.origin() + height_offset);
  views::View::ConvertPointToScreen(parent, &menu_position);
  popup_bounds_ = gfx::Rect(menu_position, element_bounds_.size());

  parent_ = parent;
  parent_->AddObserver(this);

  view_ = new AutofillPopupView(this, parent->GetWidget());
  view_->Show();

#if BUILDFLAG(ENABLE_OSR)
  if (offscreen) {
    auto* rwhv = frame_host->GetView();
    if (embedder_frame_host != nullptr) {
      rwhv = embedder_frame_host->GetView();
    }

    auto* osr_rwhv = static_cast<OffScreenRenderWidgetHostView*>(rwhv);
    view_->view_proxy_.reset(new OffscreenViewProxy(view_));
    osr_rwhv->AddViewProxy(view_->view_proxy_.get());
  }
#endif
}

void AutofillPopup::Hide() {
  if (parent_) {
    parent_->RemoveObserver(this);
    parent_ = nullptr;
  }
  if (view_) {
    view_->Hide();
    view_ = nullptr;
  }
}

void AutofillPopup::SetItems(const std::vector<base::string16>& values,
                             const std::vector<base::string16>& labels) {
  DCHECK(view_);
  values_ = values;
  labels_ = labels;
  UpdatePopupBounds();
  view_->OnSuggestionsChanged();
  if (view_)  // could be hidden after the change
    view_->DoUpdateBoundsAndRedrawPopup();
}

void AutofillPopup::AcceptSuggestion(int index) {
  frame_host_->Send(new AtomAutofillFrameMsg_AcceptSuggestion(
      frame_host_->GetRoutingID(), GetValueAt(index)));
}

void AutofillPopup::UpdatePopupBounds() {
  DCHECK(parent_);
  gfx::Point origin(element_bounds_.origin());
  views::View::ConvertPointToScreen(parent_, &origin);

  gfx::Rect bounds(origin, element_bounds_.size());
  gfx::Rect window_bounds = parent_->GetBoundsInScreen();

  popup_bounds_ =
      CalculatePopupBounds(GetDesiredPopupWidth(), GetDesiredPopupHeight(),
                           bounds, window_bounds, base::i18n::IsRTL());
}

gfx::Rect AutofillPopup::popup_bounds_in_view() {
  gfx::Point origin(popup_bounds_.origin());
  views::View::ConvertPointFromScreen(parent_, &origin);

  return gfx::Rect(origin, popup_bounds_.size());
}

void AutofillPopup::OnViewBoundsChanged(views::View* view) {
  UpdatePopupBounds();
  view_->DoUpdateBoundsAndRedrawPopup();
}

void AutofillPopup::OnViewIsDeleting(views::View* view) {
  Hide();
}

int AutofillPopup::GetDesiredPopupHeight() {
  return 2 * kPopupBorderThickness + values_.size() * kRowHeight;
}

int AutofillPopup::GetDesiredPopupWidth() {
  int popup_width = element_bounds_.width();

  for (size_t i = 0; i < values_.size(); ++i) {
    int row_size =
        kEndPadding + 2 * kPopupBorderThickness +
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
