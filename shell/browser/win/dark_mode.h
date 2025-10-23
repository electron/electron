// Copyright (c) 2022 Microsoft Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#ifndef ELECTRON_SHELL_BROWSER_WIN_DARK_MODE_H_
#define ELECTRON_SHELL_BROWSER_WIN_DARK_MODE_H_

#ifdef WIN32_LEAN_AND_MEAN
#include <Windows.h>
#else
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#undef WIN32_LEAN_AND_MEAN
#endif

namespace electron::win {

bool IsDarkModeSupported();
void SetDarkModeForWindow(HWND hWnd);

}  // namespace electron::win

#endif  // ELECTRON_SHELL_BROWSER_WIN_DARK_MODE_H_
