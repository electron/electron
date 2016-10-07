// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_system_preferences.h"

#include "base/win/wrapped_window_proc.h"
#include "ui/base/win/shell.h"
#include "ui/gfx/win/hwnd_util.h"

namespace atom {

namespace {

const wchar_t kSystemPreferencesWindowClass[] =
  L"Electron_SystemPreferencesHostWindow";

}  // namespace

namespace api {

class SystemPreferencesColorChangeListener :
    public gfx::SysColorChangeListener {
 public:
  explicit SystemPreferencesColorChangeListener(SystemPreferences* prefs):
        prefs_(prefs) {
  }

  void OnSysColorChange() {
    prefs_->OnColorChanged();
  }

 private:
  SystemPreferences* prefs_;
};

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

void SystemPreferences::InitializeWindow() {
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

  color_change_listener_.reset(
      new gfx::ScopedSysColorChangeListener(
          new SystemPreferencesColorChangeListener(this)));
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

void SystemPreferences::OnColorChanged() {
  bool new_invertered_color_scheme = IsInvertedColorScheme();
  if (new_invertered_color_scheme != invertered_color_scheme_) {
    invertered_color_scheme_ = new_invertered_color_scheme;
    Emit("inverted-color-scheme-changed", new_invertered_color_scheme);
  }
}

}  // namespace api

}  // namespace atom
