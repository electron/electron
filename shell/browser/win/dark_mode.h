// Copyright (c) 2020 Microsoft Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#ifndef SHELL_BROWSER_WIN_DARK_MODE_H_
#define SHELL_BROWSER_WIN_DARK_MODE_H_

#ifdef WIN32_LEAN_AND_MEAN
#include <Windows.h>
#else
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#undef WIN32_LEAN_AND_MEAN
#endif

namespace electron {

namespace win {

bool IsDarkModeEnabled();
bool IsDarkModeSupported();
void HandleSettingChange(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
void HandleWindowThemeChanged(HWND hWnd);
void AllowDarkModeForWindow(HWND hWnd, bool allow);
void AllowDarkModeForApp(bool allow);

}  // namespace win

}  // namespace electron

#endif  // SHELL_BROWSER_WIN_DARK_MODE_H_
