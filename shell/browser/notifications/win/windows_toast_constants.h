// Copyright (c) 2026 Electron contributors.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_NOTIFICATIONS_WIN_WINDOWS_TOAST_CONSTANTS_H_
#define ELECTRON_SHELL_BROWSER_NOTIFICATIONS_WIN_WINDOWS_TOAST_CONSTANTS_H_

namespace electron {

// Default WinRT toast Group when the app does not set groupId. Must be at most
// 16 wchar_t units on Windows 10 before Creators Update (build 15063).
inline constexpr wchar_t kWindowsToastDefaultGroup[] = L"Notifications";
inline constexpr char kWindowsToastDefaultGroupUtf8[] = "Notifications";

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_NOTIFICATIONS_WIN_WINDOWS_TOAST_CONSTANTS_H_
