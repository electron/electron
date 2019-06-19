// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_power_monitor.h"

#include <windows.h>
#include <wtsapi32.h>

#include "base/win/wrapped_window_proc.h"
#include "ui/base/win/shell.h"
#include "ui/gfx/win/hwnd_util.h"

namespace atom {

namespace {

const wchar_t kPowerMonitorWindowClass[] = L"Electron_PowerMonitorHostWindow";

}  // namespace

namespace api {

void PowerMonitor::InitPlatformSpecificMonitors() {
  WNDCLASSEX window_class;
  base::win::InitializeWindowClass(
      kPowerMonitorWindowClass,
      &base::win::WrappedWindowProc<PowerMonitor::WndProcStatic>, 0, 0, 0, NULL,
      NULL, NULL, NULL, NULL, &window_class);
  instance_ = window_class.hInstance;
  atom_ = RegisterClassEx(&window_class);

  // Create an offscreen window for receiving broadcast messages for the
  // session lock and unlock events.
  window_ = CreateWindow(MAKEINTATOM(atom_), 0, 0, 0, 0, 0, 0, HWND_MESSAGE, 0,
                         instance_, 0);
  gfx::CheckWindowCreated(window_);
  gfx::SetWindowUserData(window_, this);

  // Tel windows we want to be notified with session events
  WTSRegisterSessionNotification(window_, NOTIFY_FOR_THIS_SESSION);
}

LRESULT CALLBACK PowerMonitor::WndProcStatic(HWND hwnd,
                                             UINT message,
                                             WPARAM wparam,
                                             LPARAM lparam) {
  PowerMonitor* msg_wnd =
      reinterpret_cast<PowerMonitor*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
  if (msg_wnd)
    return msg_wnd->WndProc(hwnd, message, wparam, lparam);
  else
    return ::DefWindowProc(hwnd, message, wparam, lparam);
}

LRESULT CALLBACK PowerMonitor::WndProc(HWND hwnd,
                                       UINT message,
                                       WPARAM wparam,
                                       LPARAM lparam) {
  if (message == WM_WTSSESSION_CHANGE) {
    if (wparam == WTS_SESSION_LOCK) {
      Emit("lock-screen");
    } else if (wparam == WTS_SESSION_UNLOCK) {
      Emit("unlock-screen");
    }
  }
  return ::DefWindowProc(hwnd, message, wparam, lparam);
}

}  // namespace api

}  // namespace atom
