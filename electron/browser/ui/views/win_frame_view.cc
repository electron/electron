// Copyright (c) 2014 GitHub, Inc.
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
  return views::GetWindowBoundsForClientBounds(
      static_cast<views::View*>(const_cast<WinFrameView*>(this)),
      client_bounds);
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

}  // namespace atom
