// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chrome_process_finder_win.h"

#include <shellapi.h>
#include <string>

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/process/process.h"
#include "base/process/process_info.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/win/message_window.h"
#include "base/win/scoped_handle.h"
#include "base/win/win_util.h"
#include "base/win/windows_version.h"


namespace {

int timeout_in_milliseconds = 20 * 1000;

}  // namespace

namespace chrome {

HWND FindRunningChromeWindow(const base::FilePath& user_data_dir) {
  return base::win::MessageWindow::FindWindow(user_data_dir.value());
}

NotifyChromeResult AttemptToNotifyRunningChrome(HWND remote_window,
                                                bool fast_start) {
  DCHECK(remote_window);
  DWORD process_id = 0;
  DWORD thread_id = GetWindowThreadProcessId(remote_window, &process_id);
  if (!thread_id || !process_id)
    return NOTIFY_FAILED;

  // Send the command line to the remote chrome window.
  // Format is "START\0<<<current directory>>>\0<<<commandline>>>".
  std::wstring to_send(L"START\0", 6);  // want the NULL in the string.
  base::FilePath cur_dir;
  if (!base::GetCurrentDirectory(&cur_dir))
    return NOTIFY_FAILED;
  to_send.append(cur_dir.value());
  to_send.append(L"\0", 1);  // Null separator.
  to_send.append(::GetCommandLineW());
  to_send.append(L"\0", 1);  // Null separator.

  // Allow the current running browser window to make itself the foreground
  // window (otherwise it will just flash in the taskbar).
  ::AllowSetForegroundWindow(process_id);

  COPYDATASTRUCT cds;
  cds.dwData = 0;
  cds.cbData = static_cast<DWORD>((to_send.length() + 1) * sizeof(wchar_t));
  cds.lpData = const_cast<wchar_t*>(to_send.c_str());
  DWORD_PTR result = 0;
  if (::SendMessageTimeout(remote_window, WM_COPYDATA, NULL,
                           reinterpret_cast<LPARAM>(&cds), SMTO_ABORTIFHUNG,
                           timeout_in_milliseconds, &result)) {
    return result ? NOTIFY_SUCCESS : NOTIFY_FAILED;
  }

  // It is possible that the process owning this window may have died by now.
  if (!::IsWindow(remote_window))
    return NOTIFY_FAILED;

  // If the window couldn't be notified but still exists, assume it is hung.
  return NOTIFY_WINDOW_HUNG;
}

base::TimeDelta SetNotificationTimeoutForTesting(base::TimeDelta new_timeout) {
  base::TimeDelta old_timeout =
      base::TimeDelta::FromMilliseconds(timeout_in_milliseconds);
  timeout_in_milliseconds = new_timeout.InMilliseconds();
  return old_timeout;
}

}  // namespace chrome
