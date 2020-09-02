// Copyright (c) 2020 Microsoft Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#include "shell/browser/win/dark_mode.h"

#pragma comment(lib, "Uxtheme.lib")

// This namespace contains code from
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

using fnDwmSetWindowAttribute = HRESULT(WINAPI*)(HWND, DWORD, LPCVOID, DWORD);
fnDwmSetWindowAttribute _DwmSetWindowAttribute = {};

void RefreshTitleBarThemeColor(HWND hWnd, bool dark) {
  LONG ldark = dark;
  if (g_buildNumber >= 20161) {
    // DWMA_USE_IMMERSIVE_DARK_MODE = 20
    _DwmSetWindowAttribute(hWnd, 20, &ldark, sizeof dark);
    return;
  }
  if (g_buildNumber >= 18363) {
    auto data = WINDOWCOMPOSITIONATTRIBDATA{WCA_USEDARKMODECOLORS, &ldark,
                                            sizeof ldark};
    _SetWindowCompositionAttribute(hWnd, &data);
    return;
  }
  _DwmSetWindowAttribute(hWnd, 0x13, &ldark, sizeof ldark);
}

// helper to load symbols from a module
template <typename P>
bool Symbol(HMODULE h, P* pointer, const char* name) {
  if (P p = reinterpret_cast<P>(GetProcAddress(h, name))) {
    *pointer = p;
    return true;
  }
  return false;
}

// helper to load symbols from a module
template <typename P>
bool Symbol(HMODULE h, P* pointer, int number) {
  return Symbol(h, pointer, MAKEINTRESOURCEA(number));
}

bool CheckBuildNumber(DWORD buildNumber) {
  return (buildNumber == 17763 ||  // 1809
          buildNumber == 18362 ||  // 1903
          buildNumber == 18363 ||  // 1909
          buildNumber == 19041);   // 2004
}

void InitDarkMode() {
  HMODULE hDwmApi = LoadLibrary(L"DWMAPI");
  if (hDwmApi != 0) {
    Symbol(hDwmApi, &_DwmSetWindowAttribute, "DwmSetWindowAttribute");
  }

  // get the "get version & build numbers" function
  using fnRtlGetNtVersionNumbers = VOID(WINAPI*)(LPDWORD, LPDWORD, LPDWORD);
  auto RtlGetNtVersionNumbers = reinterpret_cast<fnRtlGetNtVersionNumbers>(
      GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "RtlGetNtVersionNumbers"));
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
  Symbol(GetModuleHandleW(L"user32.dll"), &_SetWindowCompositionAttribute,
         "SetWindowCompositionAttribute");

  // load the dark mode functions from uxtheme.dll
  // * RefreshImmersiveColorPolicyState()
  // * ShouldAppsUseDarkMode()
  // * AllowDarkModeForApp()
  // * SetPreferredAppMode()
  // * AllowDarkModeForApp() (build < 18362)
  // * SetPreferredAppMode() (build >= 18362)

  HMODULE hUxtheme =
      LoadLibraryExW(L"uxtheme.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
  if (!hUxtheme) {
    return;
  }

  using fnRefreshImmersiveColorPolicyState = VOID(WINAPI*)();  // ordinal 104
  fnRefreshImmersiveColorPolicyState _RefreshImmersiveColorPolicyState = {};
  Symbol(hUxtheme, &_RefreshImmersiveColorPolicyState, 104);

  using fnShouldAppsUseDarkMode = BOOL(WINAPI*)();  // ordinal 132
  fnShouldAppsUseDarkMode _ShouldAppsUseDarkMode = nullptr;
  Symbol(hUxtheme, &_ShouldAppsUseDarkMode, 132);

  using fnAllowDarkModeForApp =
      BOOL(WINAPI*)(BOOL allow);  // ordinal 135, in 1809
  fnAllowDarkModeForApp _AllowDarkModeForApp = {};

  typedef PreferredAppMode(WINAPI * fnSetPreferredAppMode)(
      PreferredAppMode appMode);  // ordinal 135, in 1903
  fnSetPreferredAppMode _SetPreferredAppMode = {};

  if (g_buildNumber < 18362)
    Symbol(hUxtheme, &_AllowDarkModeForApp, 135);
  else
    Symbol(hUxtheme, &_SetPreferredAppMode, 135);

  // dark mode is supported iff we found the functions
  g_darkModeSupported = _RefreshImmersiveColorPolicyState &&
                        _ShouldAppsUseDarkMode &&
                        (_AllowDarkModeForApp || _SetPreferredAppMode);
  if (!g_darkModeSupported) {
    return;
  }

  // initial setup: allow dark mode to be used
  if (_AllowDarkModeForApp)
    _AllowDarkModeForApp(true);
  else if (_SetPreferredAppMode)
    _SetPreferredAppMode(AllowDark);
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
