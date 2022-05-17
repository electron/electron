// Copyright (c) 2022 Microsoft Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#include "shell/browser/win/dark_mode.h"

#include <dwmapi.h>  // DwmSetWindowAttribute()

#include "base/win/windows_version.h"
#include "electron/fuses.h"

#define DWMWA_USE_IMMERSIVE_DARK_MODE_BEFORE_20H1 19
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20

// This namespace contains code originally from
// https://github.com/microsoft/terminal
// governed by the MIT license and (c) Microsoft Corporation.
namespace {

// Use undocumented flags to set window theme on Windows 10
HRESULT TrySetWindowThemeOnWin10(HWND hWnd, bool dark) {
  const BOOL isDarkMode = dark;
  if (FAILED(DwmSetWindowAttribute(hWnd, DWMWA_USE_IMMERSIVE_DARK_MODE,
                                   &isDarkMode, sizeof(isDarkMode)))) {
    HRESULT result =
        DwmSetWindowAttribute(hWnd, DWMWA_USE_IMMERSIVE_DARK_MODE_BEFORE_20H1,
                              &isDarkMode, sizeof(isDarkMode));
    if (FAILED(result))
      return result;
  }

  // Toggle the nonclient area active state to force a redraw (workaround for Windows 10)
  HWND activeWindow = GetActiveWindow();
  SendMessage(hWnd, WM_NCACTIVATE, hWnd != activeWindow, 0);
  SendMessage(hWnd, WM_NCACTIVATE, hWnd == activeWindow, 0);

  return S_OK;
}

// Docs: https://docs.microsoft.com/en-us/windows/win32/api/dwmapi/ne-dwmapi-dwmwindowattribute
HRESULT TrySetWindowTheme(HWND hWnd, bool dark) {
  const BOOL isDarkMode = dark;
  return DwmSetWindowAttribute(hWnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &isDarkMode, sizeof(isDarkMode));
}

}  // namespace

namespace electron {

namespace win {

bool IsDarkModeSupported() {
  auto* os_info = base::win::OSInfo::GetInstance();
  auto const version = os_info->version();

  return version >= base::win::Version::WIN11 ||
         (version >= base::win::Version::WIN10 &&
          electron::fuses::IsWindows10ImmersiveDarkModeEnabled());
}

void SetDarkModeForWindow(HWND hWnd) {
  ui::NativeTheme* theme = ui::NativeTheme::GetInstanceForNativeUi();
  bool dark =
      theme->ShouldUseDarkColors() && !theme->UserHasContrastPreference();

  auto* os_info = base::win::OSInfo::GetInstance();
  auto const version = os_info->version();

  if (version >= base::win::Version::WIN11) {
    TrySetWindowTheme(hWnd, dark);
  } else {
    TrySetWindowThemeOnWin10(hWnd, dark);
  }
}

}  // namespace win

}  // namespace electron
