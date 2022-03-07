// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/ui/win/electron_desktop_window_tree_host_win.h"

#include "base/win/windows_version.h"
#include "electron/buildflags/buildflags.h"
#include "shell/browser/ui/views/win_frame_view.h"
#include "ui/base/win/hwnd_metrics.h"
#include "ui/base/win/shell.h"

#if BUILDFLAG(ENABLE_WIN_DARK_MODE_WINDOW_UI)
#include "shell/browser/win/dark_mode.h"
#endif

namespace electron {

ElectronDesktopWindowTreeHostWin::ElectronDesktopWindowTreeHostWin(
    NativeWindowViews* native_window_view,
    views::DesktopNativeWidgetAura* desktop_native_widget_aura)
    : views::DesktopWindowTreeHostWin(native_window_view->widget(),
                                      desktop_native_widget_aura),
      native_window_view_(native_window_view) {}

ElectronDesktopWindowTreeHostWin::~ElectronDesktopWindowTreeHostWin() = default;

bool ElectronDesktopWindowTreeHostWin::PreHandleMSG(UINT message,
                                                    WPARAM w_param,
                                                    LPARAM l_param,
                                                    LRESULT* result) {
#if BUILDFLAG(ENABLE_WIN_DARK_MODE_WINDOW_UI)
  if (message == WM_NCCREATE) {
    HWND const hwnd = GetAcceleratedWidget();
    auto const theme_source =
        ui::NativeTheme::GetInstanceForNativeUi()->theme_source();
    win::SetDarkModeForWindow(hwnd, theme_source);
  }
#endif

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
  // See also https://github.com/electron/electron/issues/1821.
  return !ui::win::IsAeroGlassEnabled();
}

bool ElectronDesktopWindowTreeHostWin::GetDwmFrameInsetsInPixels(
    gfx::Insets* insets) const {
  // Set DWMFrameInsets to prevent maximized frameless window from bleeding
  // into other monitors.
  if (IsMaximized() && !native_window_view_->has_frame()) {
    // This would be equivalent to calling:
    // DwmExtendFrameIntoClientArea({0, 0, 0, 0});
    //
    // which means do not extend window frame into client area. It is almost
    // a no-op, but it can tell Windows to not extend the window frame to be
    // larger than current workspace.
    //
    // See also:
    // https://devblogs.microsoft.com/oldnewthing/20150304-00/?p=44543
    *insets = gfx::Insets();
    return true;
  }
  return false;
}

bool ElectronDesktopWindowTreeHostWin::GetClientAreaInsets(
    gfx::Insets* insets,
    HMONITOR monitor) const {
  // Windows by deafult extends the maximized window slightly larger than
  // current workspace, for frameless window since the standard frame has been
  // removed, the client area would then be drew outside current workspace.
  //
  // Indenting the client area can fix this behavior.
  if (IsMaximized() && !native_window_view_->has_frame()) {
    // The insets would be eventually passed to WM_NCCALCSIZE, which takes
    // the metrics under the DPI of _main_ monitor instead of current monitor.
    //
    // Please make sure you tested maximized frameless window under multiple
    // monitors with different DPIs before changing this code.
    const int thickness = ::GetSystemMetrics(SM_CXSIZEFRAME) +
                          ::GetSystemMetrics(SM_CXPADDEDBORDER);
    insets->Set(thickness, thickness, thickness, thickness);
    return true;
  }
  return false;
}

bool ElectronDesktopWindowTreeHostWin::HandleMouseEvent(ui::MouseEvent* event) {
  // Call the default implementation of this method to get the event to its
  // proper handler.
  bool handled = views::DesktopWindowTreeHostWin::HandleMouseEvent(event);

  // On WCO-enabled windows, we need to mark non-client mouse moved events as
  // handled so they don't incorrectly propogate back to the OS.
  if (native_window_view_->IsWindowControlsOverlayEnabled() &&
      event->type() == ui::ET_MOUSE_MOVED &&
      (event->flags() & ui::EF_IS_NON_CLIENT) != 0) {
    event->SetHandled();
    handled = true;
  }

  return handled;
}

}  // namespace electron
