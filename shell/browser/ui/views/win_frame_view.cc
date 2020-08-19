// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/ui/views/win_frame_view.h"

#include <algorithm>

#include "base/win/windows_version.h"
#include "shell/browser/native_window_views.h"
#include "ui/base/win/hwnd_metrics.h"
#include "ui/display/win/screen_win.h"
#include "ui/views/widget/widget.h"
#include "ui/views/win/hwnd_util.h"

namespace electron {

const char WinFrameView::kViewClassName[] = "WinFrameView";

WinFrameView::WinFrameView() {}

WinFrameView::~WinFrameView() {}

gfx::Insets WinFrameView::GetGlassInsets() const {
  int frame_height =
      display::win::ScreenWin::GetSystemMetricsInDIP(SM_CYSIZEFRAME) +
      display::win::ScreenWin::GetSystemMetricsInDIP(SM_CXPADDEDBORDER);

  int frame_size =
      base::win::GetVersion() < base::win::Version::WIN10
          ? display::win::ScreenWin::GetSystemMetricsInDIP(SM_CXSIZEFRAME)
          : 0;

  return gfx::Insets(frame_height, frame_size, frame_size, frame_size);
}

gfx::Insets WinFrameView::GetClientAreaInsets(HMONITOR monitor) const {
  gfx::Insets insets;
  if (base::win::GetVersion() < base::win::Version::WIN10) {
    // This tells Windows that most of the window is a client area, meaning
    // Chrome will draw it. Windows still fills in the glass bits because of the
    // DwmExtendFrameIntoClientArea call in |UpdateDWMFrame|.
    // Without this 1 pixel offset on the right and bottom:
    //   * windows paint in a more standard way, and
    //   * we get weird black bars at the top when maximized in multiple monitor
    //     configurations.
    int border_thickness = 1;
    insets.Set(0, 0, border_thickness, border_thickness);
  } else {
    const int frame_thickness = ui::GetFrameThickness(monitor);
    insets.Set(0, frame_thickness, frame_thickness, frame_thickness);
  }
  return insets;
}

gfx::Rect WinFrameView::GetWindowBoundsForClientBounds(
    const gfx::Rect& client_bounds) const {
  if (window_->IsMaximized() && !window_->has_frame()) {
    gfx::Insets insets = GetGlassInsets();
    insets += GetClientAreaInsets(
        MonitorFromWindow(HWNDForView(this), MONITOR_DEFAULTTONEAREST));
    return gfx::Rect(client_bounds.x() - insets.left(),
                     client_bounds.y() - insets.top(),
                     client_bounds.width() + insets.left() + insets.right(),
                     client_bounds.height() + insets.top() + insets.bottom());
  } else {
    return views::GetWindowBoundsForClientBounds(
        static_cast<views::View*>(const_cast<WinFrameView*>(this)),
        client_bounds);
  }
}

gfx::Rect WinFrameView::GetBoundsForClientView() const {
  if (window_->IsMaximized() && !window_->has_frame()) {
    gfx::Insets insets = GetGlassInsets();
    return gfx::Rect(insets.left(), insets.top(),
                     std::max(0, width() - insets.left() - insets.right()),
                     std::max(0, height() - insets.top() - insets.bottom()));
  } else {
    return bounds();
  }
}

gfx::Size WinFrameView::CalculatePreferredSize() const {
  gfx::Size pref = frame_->client_view()->GetPreferredSize();
  gfx::Rect bounds(0, 0, pref.width(), pref.height());
  return frame_->non_client_view()
      ->GetWindowBoundsForClientBounds(bounds)
      .size();
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
