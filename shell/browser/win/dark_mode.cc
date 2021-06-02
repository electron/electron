// Copyright (c) 2020 Microsoft Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#include "shell/browser/win/dark_mode.h"

#include <dwmapi.h>  // DwmSetWindowAttribute()

#include "base/files/file_path.h"
#include "base/scoped_native_library.h"
#include "base/win/pe_image.h"
#include "base/win/win_util.h"
#include "base/win/windows_version.h"

// This namespace contains code originally from
// https://github.com/ysc3839/win32-darkmode/
// governed by the MIT license and (c) Richard Yu
namespace {

// 1903 18362
enum PreferredAppMode { Default, AllowDark, ForceDark, ForceLight, Max };

bool g_darkModeSupported = false;
bool g_darkModeEnabled = false;
DWORD g_buildNumber = 0;

enum WINDOWCOMPOSITIONATTRIB {
  WCA_USEDARKMODECOLORS = 26  // build 18875+
};
struct WINDOWCOMPOSITIONATTRIBDATA {
  WINDOWCOMPOSITIONATTRIB Attrib;
  PVOID pvData;
  SIZE_T cbData;
};

using fnSetWindowCompositionAttribute =
    BOOL(WINAPI*)(HWND hWnd, WINDOWCOMPOSITIONATTRIBDATA*);
fnSetWindowCompositionAttribute _SetWindowCompositionAttribute = nullptr;

bool IsHighContrast() {
  HIGHCONTRASTW highContrast = {sizeof(highContrast)};
  if (SystemParametersInfoW(SPI_GETHIGHCONTRAST, sizeof(highContrast),
                            &highContrast, FALSE))
    return highContrast.dwFlags & HCF_HIGHCONTRASTON;
  return false;
}

void RefreshTitleBarThemeColor(HWND hWnd, bool dark) {
  LONG ldark = dark;
  if (g_buildNumber >= 20161) {
    // DWMA_USE_IMMERSIVE_DARK_MODE = 20
    DwmSetWindowAttribute(hWnd, 20, &ldark, sizeof dark);
    return;
  }
  if (g_buildNumber >= 18363) {
    auto data = WINDOWCOMPOSITIONATTRIBDATA{WCA_USEDARKMODECOLORS, &ldark,
                                            sizeof ldark};
    _SetWindowCompositionAttribute(hWnd, &data);
    return;
  }
  DwmSetWindowAttribute(hWnd, 0x13, &ldark, sizeof ldark);
}

void InitDarkMode() {
  // confirm that we're running on a version of Windows
  // where the Dark Mode API is known
  auto* os_info = base::win::OSInfo::GetInstance();
  g_buildNumber = os_info->version_number().build;
  auto const version = os_info->version();
  if ((version < base::win::Version::WIN10_RS5) ||
      (version > base::win::Version::WIN10_20H1)) {
    return;
  }

  // load "SetWindowCompositionAttribute", used in RefreshTitleBarThemeColor()
  _SetWindowCompositionAttribute =
      reinterpret_cast<decltype(_SetWindowCompositionAttribute)>(
          base::win::GetUser32FunctionPointer("SetWindowCompositionAttribute"));
  if (_SetWindowCompositionAttribute == nullptr) {
    return;
  }

  // load the dark mode functions from uxtheme.dll
  // * RefreshImmersiveColorPolicyState()
  // * ShouldAppsUseDarkMode()
  // * AllowDarkModeForApp()
  // * SetPreferredAppMode()
  // * AllowDarkModeForApp() (build < 18362)
  // * SetPreferredAppMode() (build >= 18362)

  base::NativeLibrary uxtheme =
      base::PinSystemLibrary(FILE_PATH_LITERAL("uxtheme.dll"));
  if (!uxtheme) {
    return;
  }
  auto ux_pei = base::win::PEImage(uxtheme);
  auto get_ux_proc_from_ordinal = [&ux_pei](int ordinal, auto* setme) {
    FARPROC proc = ux_pei.GetProcAddress(reinterpret_cast<LPCSTR>(ordinal));
    *setme = reinterpret_cast<decltype(*setme)>(proc);
  };

  // ordinal 104
  using fnRefreshImmersiveColorPolicyState = VOID(WINAPI*)();
  fnRefreshImmersiveColorPolicyState _RefreshImmersiveColorPolicyState = {};
  get_ux_proc_from_ordinal(104, &_RefreshImmersiveColorPolicyState);

  // ordinal 132
  using fnShouldAppsUseDarkMode = BOOL(WINAPI*)();
  fnShouldAppsUseDarkMode _ShouldAppsUseDarkMode = {};
  get_ux_proc_from_ordinal(132, &_ShouldAppsUseDarkMode);

  // ordinal 135, in 1809
  using fnAllowDarkModeForApp = BOOL(WINAPI*)(BOOL allow);
  fnAllowDarkModeForApp _AllowDarkModeForApp = {};

  // ordinal 135, in 1903
  typedef PreferredAppMode(WINAPI *
                           fnSetPreferredAppMode)(PreferredAppMode appMode);
  fnSetPreferredAppMode _SetPreferredAppMode = {};

  if (g_buildNumber < 18362) {
    get_ux_proc_from_ordinal(135, &_AllowDarkModeForApp);
  } else {
    get_ux_proc_from_ordinal(135, &_SetPreferredAppMode);
  }

  // dark mode is supported iff we found the functions
  g_darkModeSupported = _RefreshImmersiveColorPolicyState &&
                        _ShouldAppsUseDarkMode &&
                        (_AllowDarkModeForApp || _SetPreferredAppMode);
  if (!g_darkModeSupported) {
    return;
  }

  // initial setup: allow dark mode to be used
  if (_AllowDarkModeForApp) {
    _AllowDarkModeForApp(true);
  } else if (_SetPreferredAppMode) {
    _SetPreferredAppMode(AllowDark);
  }
  _RefreshImmersiveColorPolicyState();

  // check to see if dark mode is currently enabled
  g_darkModeEnabled = _ShouldAppsUseDarkMode() && !IsHighContrast();
}

}  // namespace

namespace electron {

void EnsureInitialized() {
  static bool initialized = false;
  if (!initialized) {
    initialized = true;
    ::InitDarkMode();
  }
}

bool IsDarkPreferred(ui::NativeTheme::ThemeSource theme_source) {
  switch (theme_source) {
    case ui::NativeTheme::ThemeSource::kForcedLight:
      return false;
    case ui::NativeTheme::ThemeSource::kForcedDark:
      return g_darkModeSupported;
    case ui::NativeTheme::ThemeSource::kSystem:
      return g_darkModeEnabled;
  }
}

namespace win {

void SetDarkModeForWindow(HWND hWnd,
                          ui::NativeTheme::ThemeSource theme_source) {
  EnsureInitialized();
  RefreshTitleBarThemeColor(hWnd, IsDarkPreferred(theme_source));
}

}  // namespace win

}  // namespace electron
