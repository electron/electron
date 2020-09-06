// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/ui/win/electron_desktop_window_tree_host_win.h"

#include "base/win/windows_version.h"
#include "shell/browser/ui/views/win_frame_view.h"
#include "ui/base/win/hwnd_metrics.h"
#include "ui/base/win/shell.h"
#include "ui/display/win/screen_win.h"
#include "ui/views/win/hwnd_util.h"

namespace electron {

ElectronDesktopWindowTreeHostWin::ElectronDesktopWindowTreeHostWin(
    NativeWindowViews* native_window_view,
    views::DesktopNativeWidgetAura* desktop_native_widget_aura)
    : views::DesktopWindowTreeHostWin(native_window_view->widget(),
                                      desktop_native_widget_aura),
      native_window_view_(native_window_view) {}

ElectronDesktopWindowTreeHostWin::~ElectronDesktopWindowTreeHostWin() {}

bool ElectronDesktopWindowTreeHostWin::PreHandleMSG(UINT message,
                                                    WPARAM w_param,
                                                    LPARAM l_param,
                                                    LRESULT* result) {
  return native_window_view_->PreHandleMSG(message, w_param, l_param, result);
}

bool ElectronDesktopWindowTreeHostWin::ShouldPaintAsActive() const {
  // Tell Chromium to use system default behavior when rendering inactive
  // titlebar, otherwise it would render inactive titlebar as active under
  // some cases.
  // See also https://github.com/electron/electron/issues/24647.
  return false;
}

bool ElectronDesktopWindowTreeHostWin::HasNativeFrame() const {
  // Since we never use chromium's titlebar implementation, we can just say
  // that we use a native titlebar. This will disable the repaint locking when
  // DWM composition is disabled.
  return !ui::win::IsAeroGlassEnabled();
}

bool ElectronDesktopWindowTreeHostWin::GetDwmFrameInsetsInPixels(
    gfx::Insets* insets) const {
  if (IsMaximized() && !native_window_view_->has_frame()) {
    HMONITOR monitor = ::MonitorFromWindow(
        native_window_view_->GetAcceleratedWidget(), MONITOR_DEFAULTTONEAREST);
    int frame_height = display::win::ScreenWin::GetSystemMetricsForMonitor(
                           monitor, SM_CYSIZEFRAME) +
                       display::win::ScreenWin::GetSystemMetricsForMonitor(
                           monitor, SM_CXPADDEDBORDER);
    int frame_size = base::win::GetVersion() < base::win::Version::WIN8
                         ? display::win::ScreenWin::GetSystemMetricsForMonitor(
                               monitor, SM_CXSIZEFRAME)
                         : 0;
    insets->Set(frame_height, frame_size, frame_size, frame_size);
    return true;
  }
  return false;
}

bool ElectronDesktopWindowTreeHostWin::GetClientAreaInsets(
    gfx::Insets* insets,
    HMONITOR monitor) const {
  if (IsMaximized() && !native_window_view_->has_frame()) {
    if (base::win::GetVersion() < base::win::Version::WIN8) {
      // This tells Windows that most of the window is a client area, meaning
      // Chrome will draw it. Windows still fills in the glass bits because of
      // the // DwmExtendFrameIntoClientArea call in |UpdateDWMFrame|.
      // Without this 1 pixel offset on the right and bottom:
      //   * windows paint in a more standard way, and
      //   * we get weird black bars at the top when maximized in multiple
      //     monitor configurations.
      int border_thickness = 1;
      insets->Set(0, 0, border_thickness, border_thickness);
    } else {
      const int frame_thickness = ui::GetFrameThickness(monitor);
      // Reduce the Windows non-client border size because we extend the border
      // into our client area in UpdateDWMFrame(). The top inset must be 0 or
      // else Windows will draw a full native titlebar outside the client area.
      insets->Set(0, frame_thickness, frame_thickness, frame_thickness);
    }
    return true;
  }
  return false;
}

}  // namespace electron
