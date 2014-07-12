// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/ui/views/win_frame_view.h"

#include "ui/views/widget/widget.h"
#include "ui/views/win/hwnd_util.h"

namespace atom {

namespace {

const char kViewClassName[] = "WinFrameView";

}  // namespace


WinFrameView::WinFrameView(views::Widget* frame)
    : frame_(frame) {
}

WinFrameView::~WinFrameView() {
}


gfx::Rect WinFrameView::GetBoundsForClientView() const {
  return gfx::Rect(0, 0, width(), height());
}

gfx::Rect WinFrameView::GetWindowBoundsForClientBounds(
    const gfx::Rect& client_bounds) const {
  gfx::Size size(client_bounds.size());
  ClientAreaSizeToWindowSize(&size);
  return gfx::Rect(client_bounds.origin(), size);
}

int WinFrameView::NonClientHitTest(const gfx::Point& point) {
  return frame_->client_view()->NonClientHitTest(point);
}

void WinFrameView::GetWindowMask(const gfx::Size& size,
                                 gfx::Path* window_mask) {
  // Nothing to do, we use the default window mask.
}

void WinFrameView::ResetWindowControls() {
  // Nothing to do.
}

void WinFrameView::UpdateWindowIcon() {
  // Nothing to do.
}

void WinFrameView::UpdateWindowTitle() {
  // Nothing to do.
}

gfx::Size WinFrameView::GetPreferredSize() {
  gfx::Size client_preferred_size = frame_->client_view()->GetPreferredSize();
  return frame_->non_client_view()->GetWindowBoundsForClientBounds(
      gfx::Rect(client_preferred_size)).size();
}

gfx::Size WinFrameView::GetMinimumSize() {
  return frame_->client_view()->GetMinimumSize();
}

gfx::Size WinFrameView::GetMaximumSize() {
  return frame_->client_view()->GetMaximumSize();
}

const char* WinFrameView::GetClassName() const {
  return kViewClassName;
}

void WinFrameView::ClientAreaSizeToWindowSize(gfx::Size* size) const {
  // AdjustWindowRect seems to return a wrong window size.
  gfx::Size window = frame_->GetWindowBoundsInScreen().size();
  gfx::Size client = frame_->GetClientAreaBoundsInScreen().size();
  size->set_width(size->width() + window.width() - client.width());
  size->set_height(size->height() + window.height() - client.height());
}

}  // namespace atom
