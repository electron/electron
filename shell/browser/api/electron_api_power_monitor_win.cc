// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/electron_api_power_monitor.h"

#include <windows.h>
#include <wtsapi32.h>

#include "base/logging.h"
#include "base/win/windows_types.h"
#include "base/win/wrapped_window_proc.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "ui/gfx/win/hwnd_util.h"

namespace electron {

namespace {

const wchar_t kPowerMonitorWindowClass[] = L"Electron_PowerMonitorHostWindow";

}  // namespace

namespace api {

void PowerMonitor::InitPlatformSpecificMonitors() {
  WNDCLASSEX window_class;
  base::win::InitializeWindowClass(
      kPowerMonitorWindowClass,
      &base::win::WrappedWindowProc<PowerMonitor::WndProcStatic>, 0, 0, 0,
      nullptr, nullptr, nullptr, nullptr, nullptr, &window_class);
  instance_ = window_class.hInstance;
  atom_ = RegisterClassEx(&window_class);

  // Create an offscreen window for receiving broadcast messages for the
  // session lock and unlock events.
  window_ = CreateWindow(MAKEINTATOM(atom_), 0, 0, 0, 0, 0, 0, HWND_MESSAGE, 0,
                         instance_, 0);
  gfx::CheckWindowCreated(window_, ::GetLastError());
  gfx::SetWindowUserData(window_, this);

  // Tel windows we want to be notified with session events
  WTSRegisterSessionNotification(window_, NOTIFY_FOR_THIS_SESSION);

  // For Windows 8 and later, a new "connected standby" mode has been added and
  // we must explicitly register for its notifications.
  RegisterSuspendResumeNotification(static_cast<HANDLE>(window_),
                                    DEVICE_NOTIFY_WINDOW_HANDLE);
}

LRESULT CALLBACK PowerMonitor::WndProcStatic(HWND hwnd,
                                             UINT message,
                                             WPARAM wparam,
                                             LPARAM lparam) {
  auto* msg_wnd =
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
    bool should_treat_as_current_session = true;
    DWORD current_session_id = 0;
    if (!::ProcessIdToSessionId(::GetCurrentProcessId(), &current_session_id)) {
      LOG(ERROR) << "ProcessIdToSessionId failed, assuming current session";
    } else {
      should_treat_as_current_session =
          (static_cast<DWORD>(lparam) == current_session_id);
    }
    if (should_treat_as_current_session) {
      if (wparam == WTS_SESSION_LOCK) {
        // Unretained is OK because this object is eternally pinned.
        content::GetUIThreadTaskRunner({})->PostTask(
            FROM_HERE,
            base::BindOnce([](PowerMonitor* pm) { pm->Emit("lock-screen"); },
                           base::Unretained(this)));
      } else if (wparam == WTS_SESSION_UNLOCK) {
        content::GetUIThreadTaskRunner({})->PostTask(
            FROM_HERE,
            base::BindOnce([](PowerMonitor* pm) { pm->Emit("unlock-screen"); },
                           base::Unretained(this)));
      }
    }
  } else if (message == WM_POWERBROADCAST) {
    if (wparam == PBT_APMRESUMEAUTOMATIC) {
      content::GetUIThreadTaskRunner({})->PostTask(
          FROM_HERE,
          base::BindOnce([](PowerMonitor* pm) { pm->Emit("resume"); },
                         base::Unretained(this)));
    } else if (wparam == PBT_APMSUSPEND) {
      content::GetUIThreadTaskRunner({})->PostTask(
          FROM_HERE,
          base::BindOnce([](PowerMonitor* pm) { pm->Emit("suspend"); },
                         base::Unretained(this)));
    }
  }
  return ::DefWindowProc(hwnd, message, wparam, lparam);
}

}  // namespace api

}  // namespace electron
