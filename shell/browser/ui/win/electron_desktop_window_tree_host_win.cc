// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/ui/win/electron_desktop_window_tree_host_win.h"

#include "base/win/windows_version.h"
#include "electron/buildflags/buildflags.h"
#include "shell/browser/ui/views/win_frame_view.h"
#include "shell/browser/win/dark_mode.h"
#include "ui/base/win/hwnd_metrics.h"
#include "ui/base/win/shell.h"

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
  const bool dark_mode_supported = win::IsDarkModeSupported();
  if (dark_mode_supported && message == WM_NCCREATE) {
    win::SetDarkModeForWindow(GetAcceleratedWidget());
    ui::NativeTheme::GetInstanceForNativeUi()->AddObserver(this);
  } else if (dark_mode_supported && message == WM_DESTROY) {
    ui::NativeTheme::GetInstanceForNativeUi()->RemoveObserver(this);
  }

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
  // Windows by default extends the maximized window slightly larger than
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
    *insets = gfx::Insets::TLBR(thickness, thickness, thickness, thickness);
    return true;
  }
  return false;
}

bool ElectronDesktopWindowTreeHostWin::HandleMouseEventForCaption(
    UINT message) const {
  // Windows does not seem to generate WM_NCPOINTERDOWN/UP touch events for
  // caption buttons with WCO. This results in a no-op for
  // HWNDMessageHandler::HandlePointerEventTypeTouchOrNonClient and
  // WM_SYSCOMMAND is not generated for the touch action. However, Windows will
  // also generate a mouse event for every touch action and this gets handled in
  // HWNDMessageHandler::HandleMouseEventInternal.
  // With https://chromium-review.googlesource.com/c/chromium/src/+/1048877/
  // Chromium lets the OS handle caption buttons for FrameMode::SYSTEM_DRAWN but
  // again this does not generate the SC_MINIMIZE, SC_MAXIMIZE, SC_RESTORE
  // commands when Non-client mouse events are generated for HTCLOSE,
  // HTMINBUTTON, HTMAXBUTTON. To workaround this issue, wit this delegate we
  // let chromium handle the mouse events via
  // HWNDMessageHandler::HandleMouseInputForCaption instead of the OS and this
  // will generate the necessary system commands to perform caption button
  // actions due to the DefWindowProc call.
  // https://source.chromium.org/chromium/chromium/src/+/main:ui/views/win/hwnd_message_handler.cc;l=3611
  return native_window_view_->IsWindowControlsOverlayEnabled();
}

void ElectronDesktopWindowTreeHostWin::OnNativeThemeUpdated(
    ui::NativeTheme* observed_theme) {
  win::SetDarkModeForWindow(GetAcceleratedWidget());
}

}  // namespace electron
