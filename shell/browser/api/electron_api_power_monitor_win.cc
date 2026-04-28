// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/electron_api_power_monitor.h"

#include <windows.h>
#include <wtsapi32.h>

#include "base/debug/alias.h"
#include "base/debug/crash_logging.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/win/windows_handle_util.h"
#include "base/win/windows_types.h"
#include "base/win/wrapped_window_proc.h"
#include "components/crash/core/common/crash_key.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "ui/gfx/win/hwnd_util.h"

namespace electron {

namespace {

const wchar_t kPowerMonitorWindowClass[] = L"Electron_PowerMonitorHostWindow";

std::string DescribeMemoryState(void* address) {
  MEMORY_BASIC_INFORMATION mbi = {};
  if (!VirtualQuery(address, &mbi, sizeof(mbi))) {
    return "VirtualQuery failed err=" + base::NumberToString(::GetLastError());
  }
  // Refs
  // https://learn.microsoft.com/en-us/windows/win32/api/winnt/ns-winnt-memory_basic_information
  return "state=" + base::NumberToString(mbi.State) +
         " protect=" + base::NumberToString(mbi.Protect) +
         " type=" + base::NumberToString(mbi.Type);
}

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
  power_notify_handle_ = RegisterSuspendResumeNotification(
      static_cast<HANDLE>(window_), DEVICE_NOTIFY_WINDOW_HANDLE);
  PLOG_IF(ERROR, !power_notify_handle_)
      << "RegisterSuspendResumeNotification failed";

  // On ARM64 Windows, UnregisterSuspendResumeNotification may
  // crash in powrprof!PowerUnregisterSuspendResumeNotification by dereferencing
  // the HPOWERNOTIFY handle. VirtualQuery on the handle address reveals
  // whether it was ever a valid pointer.
  // Use static crash keys (not SCOPED_) so they persist until the crash.
  if (power_notify_handle_) {
    static crash_reporter::CrashKeyString<16> reg_handle_key("pm-reg-handle");
    static crash_reporter::CrashKeyString<64> reg_memstate_key(
        "pm-reg-memstate");
    reg_handle_key.Set(
        base::NumberToString(base::win::HandleToUint32(power_notify_handle_)));
    reg_memstate_key.Set(DescribeMemoryState(power_notify_handle_));
  }
}

void PowerMonitor::DestroyPlatformSpecificMonitors() {
  if (window_) {
    WTSUnRegisterSessionNotification(window_);
    if (power_notify_handle_) {
      // Capture handle value and memory state at unregistration time.
      // debug::Alias forces the raw value onto the stack.
      auto handle_value = base::win::HandleToUint32(power_notify_handle_);
      base::debug::Alias(&handle_value);

      static crash_reporter::CrashKeyString<64> unreg_memstate_key(
          "pm-unreg-memstate");
      unreg_memstate_key.Set(DescribeMemoryState(power_notify_handle_));

      UnregisterSuspendResumeNotification(power_notify_handle_);
      power_notify_handle_ = nullptr;
    }
    gfx::SetWindowUserData(window_, nullptr);
    DestroyWindow(window_);
    window_ = nullptr;
  }
  if (atom_) {
    UnregisterClass(MAKEINTATOM(atom_), instance_);
    atom_ = 0;
  }
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
        // JS module reference prevents GC of this object, so Unretained is
        // safe.
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
  }
  return ::DefWindowProc(hwnd, message, wparam, lparam);
}

}  // namespace api

}  // namespace electron
