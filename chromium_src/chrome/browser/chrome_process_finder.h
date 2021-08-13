// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROME_PROCESS_FINDER_H_
#define CHROME_BROWSER_CHROME_PROCESS_FINDER_H_

#include <windows.h>
#include <string>

#include "base/time/time.h"

namespace base {
class FilePath;
}

namespace chrome {

enum NotifyChromeResult {
  NOTIFY_SUCCESS,
  NOTIFY_FAILED,
  NOTIFY_WINDOW_HUNG,
};

// Finds an already running Chrome window if it exists.
HWND FindRunningChromeWindow(const base::FilePath& user_data_dir);

// Attempts to send the current command line to an already running instance of
// Chrome via a WM_COPYDATA message.
// Returns true if a running Chrome is found and successfully notified.
NotifyChromeResult AttemptToNotifyRunningChrome(
    HWND remote_window,
    const std::wstring& additional_data);

// Changes the notification timeout to |new_timeout|, returns the old timeout.
base::TimeDelta SetNotificationTimeoutForTesting(base::TimeDelta new_timeout);

}  // namespace chrome

#endif  // CHROME_BROWSER_CHROME_PROCESS_FINDER_H_
