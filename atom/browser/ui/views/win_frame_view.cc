// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/ui/views/win_frame_view.h"

#include "atom/browser/native_window_views.h"
#include "ui/views/widget/widget.h"
#include "ui/views/win/hwnd_util.h"

namespace atom {

namespace {

const char kViewClassName[] = "WinFrameView";

}  // namespace


WinFrameView::WinFrameView() {
}

WinFrameView::~WinFrameView() {
}


gfx::Rect WinFrameView::GetWindowBoundsForClientBounds(
    const gfx::Rect& client_bounds) const {
  gfx::Size size(client_bounds.size());
  ClientAreaSizeToWindowSize(&size);
  return gfx::Rect(client_bounds.origin(), size);
}

int WinFrameView::NonClientHitTest(const gfx::Point& point) {
  if (window_->has_frame())
    return frame_->client_view()->NonClientHitTest(point);
  else
    return FramelessView::NonClientHitTest(point);
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
