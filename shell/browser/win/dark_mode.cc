// Copyright (c) 2020 Microsoft Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#include "shell/browser/win/dark_mode.h"

#include <dwmapi.h>  // DwmSetWindowAttribute()

#include "base/files/file_path.h"
#include "base/scoped_native_library.h"
#include "base/win/pe_image.h"
#include "base/win/win_util.h"

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

bool CheckBuildNumber(DWORD buildNumber) {
  return (buildNumber == 17763 ||  // 1809
          buildNumber == 18362 ||  // 1903
          buildNumber == 18363 ||  // 1909
          buildNumber == 19041);   // 2004
}

void InitDarkMode() {
  // get the "get version & build numbers" function
  auto nt_snl =
      base::ScopedNativeLibrary(base::FilePath(FILE_PATH_LITERAL("ntdll.dll")));
  auto nt_pei = base::win::PEImage(nt_snl.get());
  using fnRtlGetNtVersionNumbers = VOID(WINAPI*)(LPDWORD, LPDWORD, LPDWORD);
  auto RtlGetNtVersionNumbers = reinterpret_cast<fnRtlGetNtVersionNumbers>(
      nt_pei.GetProcAddress("RtlGetNtVersionNumbers"));
  if (!RtlGetNtVersionNumbers) {
    return;
  }

  // check to see if the major/minor/build support these dark mode APIS
  DWORD major = 0, minor = 0;
  RtlGetNtVersionNumbers(&major, &minor, &g_buildNumber);
  g_buildNumber &= ~0xF0000000;
  if (major != 10 || minor != 0 || !CheckBuildNumber(g_buildNumber)) {
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
