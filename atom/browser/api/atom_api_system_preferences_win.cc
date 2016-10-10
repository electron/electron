// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_system_preferences.h"

#include "atom/common/color_util.h"
#include "base/win/wrapped_window_proc.h"
#include "ui/base/win/shell.h"
#include "ui/gfx/color_utils.h"
#include "ui/gfx/win/hwnd_util.h"

namespace atom {

namespace {

const wchar_t kSystemPreferencesWindowClass[] =
  L"Electron_SystemPreferencesHostWindow";

}  // namespace

namespace api {

bool SystemPreferences::IsAeroGlassEnabled() {
  return ui::win::IsAeroGlassEnabled();
}

std::string hexColorDWORDToRGBA(DWORD color) {
  std::ostringstream stream;
  stream << std::hex << color;
  std::string hexColor = stream.str();
  return hexColor.substr(2) + hexColor.substr(0, 2);
}

std::string SystemPreferences::GetAccentColor() {
  DWORD color = 0;
  BOOL opaque = FALSE;

  if (FAILED(dwmGetColorizationColor(&color, &opaque))) {
    return "";
  }

  return hexColorDWORDToRGBA(color);
}

std::string SystemPreferences::GetColor(const std::string& color,
                                        mate::Arguments* args) {
  int id;
  if (color == "3d-dark-shadow") {
    id = COLOR_3DDKSHADOW;
  } else if (color == "3d-face") {
    id = COLOR_3DFACE;
  } else if (color == "3d-highlight") {
    id = COLOR_3DHIGHLIGHT;
  } else if (color == "3d-light") {
    id = COLOR_3DLIGHT;
  } else if (color == "3d-shadow") {
    id = COLOR_3DSHADOW;
  } else if (color == "active-border") {
    id = COLOR_ACTIVEBORDER;
  } else if (color == "active-caption") {
    id = COLOR_ACTIVECAPTION;
  } else if (color == "active-caption-gradient") {
    id = COLOR_GRADIENTACTIVECAPTION;
  } else if (color == "app-workspace") {
    id = COLOR_APPWORKSPACE;
  } else if (color == "background") {
    id = COLOR_BACKGROUND;
  } else if (color == "button-face") {
    id = COLOR_BTNFACE;
  } else if (color == "button-highlight") {
    id = COLOR_BTNHIGHLIGHT;
  } else if (color == "button-shadow") {
    id = COLOR_BTNSHADOW;
  } else if (color == "button-text") {
    id = COLOR_BTNTEXT;
  } else if (color == "caption-text") {
    id = COLOR_CAPTIONTEXT;
  } else if (color == "desktop") {
    id = COLOR_DESKTOP;
  } else if (color == "disabled-text") {
    id = COLOR_GRAYTEXT;
  } else if (color == "highlight") {
    id = COLOR_HIGHLIGHT;
  } else if (color == "highlight-text") {
    id = COLOR_HIGHLIGHTTEXT;
  } else if (color == "hotlight") {
    id = COLOR_HOTLIGHT;
  } else if (color == "inactive-border") {
    id = COLOR_INACTIVEBORDER;
  } else if (color == "inactive-caption") {
    id = COLOR_INACTIVECAPTION;
  } else if (color == "inactive-caption-gradient") {
    id = COLOR_GRADIENTINACTIVECAPTION;
  } else if (color == "inactive-caption-text") {
    id = COLOR_INACTIVECAPTIONTEXT;
  } else if (color == "info-background") {
    id = COLOR_INFOBK;
  } else if (color == "info-text") {
    id = COLOR_INFOTEXT;
  } else if (color == "menu") {
    id = COLOR_MENU;
  } else if (color == "menu-highlight") {
    id = COLOR_MENUHILIGHT;
  } else if (color == "menubar") {
    id = COLOR_MENUBAR;
  } else if (color == "menu-text") {
    id = COLOR_MENUTEXT;
  } else if (color == "scrollbar") {
    id = COLOR_SCROLLBAR;
  } else if (color == "window") {
    id = COLOR_WINDOW;
  } else if (color == "window-frame") {
    id = COLOR_WINDOWFRAME;
  } else if (color == "window-text") {
    id = COLOR_WINDOWTEXT;
  } else {
    args->ThrowError("Unknown color: " + color);
    return "";
  }

  return ToRGBHex(color_utils::GetSysSkColor(id));
}

void SystemPreferences::InitializeWindow() {
  invertered_color_scheme_ = IsInvertedColorScheme();

  WNDCLASSEX window_class;
  base::win::InitializeWindowClass(
      kSystemPreferencesWindowClass,
      &base::win::WrappedWindowProc<SystemPreferences::WndProcStatic>,
      0, 0, 0, NULL, NULL, NULL, NULL, NULL,
      &window_class);
  instance_ = window_class.hInstance;
  atom_ = RegisterClassEx(&window_class);

  // Create an offscreen window for receiving broadcast messages for the system
  // colorization color.  Create a hidden WS_POPUP window instead of an
  // HWND_MESSAGE window, because only top-level windows such as popups can
  // receive broadcast messages like "WM_DWMCOLORIZATIONCOLORCHANGED".
  window_ = CreateWindow(MAKEINTATOM(atom_),
                         0, WS_POPUP, 0, 0, 0, 0, 0, 0, instance_, 0);
  gfx::CheckWindowCreated(window_);
  gfx::SetWindowUserData(window_, this);
}

LRESULT CALLBACK SystemPreferences::WndProcStatic(HWND hwnd,
                                              UINT message,
                                              WPARAM wparam,
                                              LPARAM lparam) {
  SystemPreferences* msg_wnd = reinterpret_cast<SystemPreferences*>(
      GetWindowLongPtr(hwnd, GWLP_USERDATA));
  if (msg_wnd)
    return msg_wnd->WndProc(hwnd, message, wparam, lparam);
  else
    return ::DefWindowProc(hwnd, message, wparam, lparam);
}

LRESULT CALLBACK SystemPreferences::WndProc(HWND hwnd,
                                        UINT message,
                                        WPARAM wparam,
                                        LPARAM lparam) {
  if (message == WM_DWMCOLORIZATIONCOLORCHANGED) {
    DWORD new_color = (DWORD) wparam;
    std::string new_color_string = hexColorDWORDToRGBA(new_color);
    if (new_color_string != current_color_) {
      Emit("accent-color-changed", hexColorDWORDToRGBA(new_color));
      current_color_ = new_color_string;
    }
  }
  return ::DefWindowProc(hwnd, message, wparam, lparam);
}

void SystemPreferences::OnSysColorChange() {
  bool new_invertered_color_scheme = IsInvertedColorScheme();
  if (new_invertered_color_scheme != invertered_color_scheme_) {
    invertered_color_scheme_ = new_invertered_color_scheme;
    Emit("inverted-color-scheme-changed", new_invertered_color_scheme);
  }
}

}  // namespace api

}  // namespace atom
