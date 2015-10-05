// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/ui/views/frameless_view.h"

#include "atom/browser/native_window_views.h"
#include "ui/aura/window.h"
#include "ui/base/hit_test.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_delegate.h"

namespace atom {

namespace {

const int kResizeInsideBoundsSize = 5;
const int kResizeAreaCornerSize = 16;

const char kViewClassName[] = "FramelessView";

}  // namespace

FramelessView::FramelessView() : window_(NULL), frame_(NULL) {
}

FramelessView::~FramelessView() {
}

void FramelessView::Init(NativeWindowViews* window, views::Widget* frame) {
  window_ = window;
  frame_ = frame;
}

int FramelessView::ResizingBorderHitTest(const gfx::Point& point) {
  // Check the frame first, as we allow a small area overlapping the contents
  // to be used for resize handles.
  bool can_ever_resize = frame_->widget_delegate() ?
      frame_->widget_delegate()->CanResize() :
      false;
  // Don't allow overlapping resize handles when the window is maximized or
  // fullscreen, as it can't be resized in those states.
  int resize_border =
      frame_->IsMaximized() || frame_->IsFullscreen() ? 0 :
      kResizeInsideBoundsSize;
  return GetHTComponentForFrame(point, resize_border, resize_border,
      kResizeAreaCornerSize, kResizeAreaCornerSize, can_ever_resize);
}

gfx::Rect FramelessView::GetBoundsForClientView() const {
  return bounds();
}

gfx::Rect FramelessView::GetWindowBoundsForClientBounds(
    const gfx::Rect& client_bounds) const {
  gfx::Rect window_bounds = client_bounds;
  // Enforce minimum size (1, 1) in case that client_bounds is passed with
  // empty size. This could occur when the frameless window is being
  // initialized.
  if (window_bounds.IsEmpty()) {
    window_bounds.set_width(1);
    window_bounds.set_height(1);
  }
  return window_bounds;
}

int FramelessView::NonClientHitTest(const gfx::Point& cursor) {
  if (frame_->IsFullscreen())
    return HTCLIENT;

  // Check for possible draggable region in the client area for the frameless
  // window.
  SkRegion* draggable_region = window_->draggable_region();
  if (draggable_region && draggable_region->contains(cursor.x(), cursor.y()))
    return HTCAPTION;

  // Support resizing frameless window by dragging the border.
  int frame_component = ResizingBorderHitTest(cursor);
  if (frame_component != HTNOWHERE)
    return frame_component;

  return HTCLIENT;
}

void FramelessView::GetWindowMask(const gfx::Size& size,
                                  gfx::Path* window_mask) {
}

void FramelessView::ResetWindowControls() {
}

void FramelessView::UpdateWindowIcon() {
}

void FramelessView::UpdateWindowTitle() {
}

void FramelessView::SizeConstraintsChanged() {
}

gfx::Size FramelessView::GetPreferredSize() const {
  return frame_->non_client_view()->GetWindowBoundsForClientBounds(
      gfx::Rect(frame_->client_view()->GetPreferredSize())).size();
}

gfx::Size FramelessView::GetMinimumSize() const {
  return window_->GetContentSizeConstraints().GetMinimumSize();
}

gfx::Size FramelessView::GetMaximumSize() const {
  return window_->GetContentSizeConstraints().GetMaximumSize();
}

const char* FramelessView::GetClassName() const {
  return kViewClassName;
}

}  // namespace atom
