// Copyright (c) 2022 Microsoft Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#include "shell/browser/win/dark_mode.h"

#include <dwmapi.h>  // DwmSetWindowAttribute()

#define DARK_MODE_STRING_NAME L"DarkMode_Explorer"
#define DWMWA_USE_IMMERSIVE_DARK_MODE_BEFORE_20H1 19
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20

// This namespace contains code originally from
// https://github.com/microsoft/terminal
// governed by the MIT license and (c) Microsoft Corporation.
namespace {

// Use private Windows API to set window theme.
HRESULT TrySetWindowTheme(HWND hWnd, bool dark) {
  HRESULT result =
      SetWindowTheme(hWnd, dark ? DARK_MODE_STRING_NAME : L"", nullptr);
  if (FAILED(result))
    return result;

  const BOOL isDarkMode = dark;
  if (FAILED(DwmSetWindowAttribute(hWnd, DWMWA_USE_IMMERSIVE_DARK_MODE,
                                   &isDarkMode, sizeof(isDarkMode)))) {
    result =
        DwmSetWindowAttribute(hWnd, DWMWA_USE_IMMERSIVE_DARK_MODE_BEFORE_20H1,
                              &isDarkMode, sizeof(isDarkMode));
    if (FAILED(result))
      return result;
  }

  return S_OK;
}

}  // namespace

namespace electron {

namespace win {

void SetDarkModeForWindow(HWND hWnd) {
  ui::NativeTheme* theme = ui::NativeTheme::GetInstanceForNativeUi();
  bool dark =
      theme->ShouldUseDarkColors() && !theme->UserHasContrastPreference();
  TrySetWindowTheme(hWnd, dark);
}

}  // namespace win

}  // namespace electron
