// Copyright (c) 2020 Microsoft Inc. All rights reserved.
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

#include "ui/native_theme/native_theme.h"

namespace electron {

namespace win {

void SetDarkModeForWindow(HWND hWnd, ui::NativeTheme::ThemeSource theme_source);

}  // namespace win

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_WIN_DARK_MODE_H_
