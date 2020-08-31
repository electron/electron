// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/ui/views/win_frame_view.h"

#include "base/win/windows_version.h"
#include "shell/browser/native_window_views.h"
#include "ui/display/win/screen_win.h"
#include "ui/views/widget/widget.h"
#include "ui/views/win/hwnd_util.h"

namespace electron {

namespace {

gfx::Insets GetGlassInsets() {
  int frame_height =
      display::win::ScreenWin::GetSystemMetricsInDIP(SM_CYSIZEFRAME) +
      display::win::ScreenWin::GetSystemMetricsInDIP(SM_CXPADDEDBORDER);

  int frame_size =
      base::win::GetVersion() < base::win::Version::WIN8
          ? display::win::ScreenWin::GetSystemMetricsInDIP(SM_CXSIZEFRAME)
          : 0;

  return gfx::Insets(frame_height, frame_size, frame_size, frame_size);
}

}  // namespace

const char WinFrameView::kViewClassName[] = "WinFrameView";

WinFrameView::WinFrameView() {}

WinFrameView::~WinFrameView() {}

gfx::Rect WinFrameView::GetWindowBoundsForClientBounds(
    const gfx::Rect& client_bounds) const {
  return views::GetWindowBoundsForClientBounds(
      static_cast<views::View*>(const_cast<WinFrameView*>(this)),
      client_bounds);
}

gfx::Rect WinFrameView::GetBoundsForClientView() const {
  if (window_->IsMaximized() && !window_->has_frame()) {
    gfx::Insets insets = GetGlassInsets();
    gfx::Rect result(width(), height());
    result.Inset(insets);
    return result;
  } else {
    return bounds();
  }
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

}  // namespace electron
