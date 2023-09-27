// Copyright (c) 2022 Microsoft Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#include "shell/browser/win/dark_mode.h"

#include <dwmapi.h>  // DwmSetWindowAttribute()

#include "base/win/windows_version.h"

// This flag works since Win10 20H1 but is not documented until Windows 11
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20

// This namespace contains code originally from
// https://github.com/microsoft/terminal
// governed by the MIT license and (c) Microsoft Corporation.
namespace {

// https://docs.microsoft.com/en-us/windows/win32/api/dwmapi/ne-dwmapi-dwmwindowattribute
HRESULT TrySetWindowTheme(HWND hWnd, bool dark) {
  const BOOL isDarkMode = dark;
  HRESULT result = DwmSetWindowAttribute(hWnd, DWMWA_USE_IMMERSIVE_DARK_MODE,
                                         &isDarkMode, sizeof(isDarkMode));

  if (FAILED(result))
    return result;

  return S_OK;
}

}  // namespace

namespace electron::win {

bool IsDarkModeSupported() {
  auto* os_info = base::win::OSInfo::GetInstance();
  auto const version = os_info->version();

  return version >= base::win::Version::WIN10_20H1;
}

void SetDarkModeForWindow(HWND hWnd) {
  ui::NativeTheme* theme = ui::NativeTheme::GetInstanceForNativeUi();
  bool dark =
      theme->ShouldUseDarkColors() && !theme->UserHasContrastPreference();

  TrySetWindowTheme(hWnd, dark);
}

}  // namespace electron::win
