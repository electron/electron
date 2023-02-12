// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <dwmapi.h>
#include <windows.devices.enumeration.h>
#include <wrl/client.h>
#include <iomanip>

#include "shell/browser/api/electron_api_system_preferences.h"

#include "base/win/core_winrt_util.h"
#include "base/win/windows_types.h"
#include "base/win/wrapped_window_proc.h"
#include "shell/common/color_util.h"
#include "ui/base/win/shell.h"
#include "ui/gfx/color_utils.h"
#include "ui/gfx/win/hwnd_util.h"

namespace electron {

namespace {

const wchar_t kSystemPreferencesWindowClass[] =
    L"Electron_SystemPreferencesHostWindow";

using ABI::Windows::Devices::Enumeration::DeviceAccessStatus;
using ABI::Windows::Devices::Enumeration::DeviceClass;
using ABI::Windows::Devices::Enumeration::IDeviceAccessInformation;
using ABI::Windows::Devices::Enumeration::IDeviceAccessInformationStatics;
using Microsoft::WRL::ComPtr;

DeviceAccessStatus GetDeviceAccessStatus(DeviceClass device_class) {
  ComPtr<IDeviceAccessInformationStatics> dev_access_info_statics;
  HRESULT hr = base::win::GetActivationFactory<
      IDeviceAccessInformationStatics,
      RuntimeClass_Windows_Devices_Enumeration_DeviceAccessInformation>(
      &dev_access_info_statics);
  if (FAILED(hr)) {
    VLOG(1) << "IDeviceAccessInformationStatics failed: " << hr;
    return DeviceAccessStatus::DeviceAccessStatus_Allowed;
  }

  ComPtr<IDeviceAccessInformation> dev_access_info;
  hr = dev_access_info_statics->CreateFromDeviceClass(device_class,
                                                      &dev_access_info);
  if (FAILED(hr)) {
    VLOG(1) << "IDeviceAccessInformation failed: " << hr;
    return DeviceAccessStatus::DeviceAccessStatus_Allowed;
  }

  auto status = DeviceAccessStatus::DeviceAccessStatus_Unspecified;
  dev_access_info->get_CurrentStatus(&status);
  return status;
}

std::string ConvertDeviceAccessStatus(DeviceAccessStatus value) {
  switch (value) {
    case DeviceAccessStatus::DeviceAccessStatus_Unspecified:
      return "not-determined";
    case DeviceAccessStatus::DeviceAccessStatus_Allowed:
      return "granted";
    case DeviceAccessStatus::DeviceAccessStatus_DeniedBySystem:
      return "restricted";
    case DeviceAccessStatus::DeviceAccessStatus_DeniedByUser:
      return "denied";
    default:
      return "unknown";
  }
}

}  // namespace

namespace api {

bool SystemPreferences::IsAeroGlassEnabled() {
  return ui::win::IsAeroGlassEnabled();
}

std::string hexColorDWORDToRGBA(DWORD color) {
  DWORD rgba = color << 8 | color >> 24;
  std::ostringstream stream;
  stream << std::hex << std::setw(8) << std::setfill('0') << rgba;
  return stream.str();
}

std::string SystemPreferences::GetAccentColor() {
  DWORD color = 0;
  BOOL opaque = FALSE;

  if (FAILED(DwmGetColorizationColor(&color, &opaque))) {
    return "";
  }

  return hexColorDWORDToRGBA(color);
}

std::string SystemPreferences::GetColor(gin_helper::ErrorThrower thrower,
                                        const std::string& color) {
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
    thrower.ThrowError("Unknown color: " + color);
    return "";
  }

  return ToRGBHex(color_utils::GetSysSkColor(id));
}

std::string SystemPreferences::GetMediaAccessStatus(
    gin_helper::ErrorThrower thrower,
    const std::string& media_type) {
  if (media_type == "camera") {
    return ConvertDeviceAccessStatus(
        GetDeviceAccessStatus(DeviceClass::DeviceClass_VideoCapture));
  } else if (media_type == "microphone") {
    return ConvertDeviceAccessStatus(
        GetDeviceAccessStatus(DeviceClass::DeviceClass_AudioCapture));
  } else if (media_type == "screen") {
    return ConvertDeviceAccessStatus(
        DeviceAccessStatus::DeviceAccessStatus_Allowed);
  } else {
    thrower.ThrowError("Invalid media type");
    return std::string();
  }
}

void SystemPreferences::InitializeWindow() {
  inverted_color_scheme_ = IsInvertedColorScheme();
  high_contrast_color_scheme_ = IsHighContrastColorScheme();

  // Wait until app is ready before creating sys color listener
  // Creating this listener before the app is ready causes global shortcuts
  // to not fire
  if (Browser::Get()->is_ready())
    color_change_listener_ =
        std::make_unique<gfx::ScopedSysColorChangeListener>(this);
  else
    Browser::Get()->AddObserver(this);

  WNDCLASSEX window_class;
  base::win::InitializeWindowClass(
      kSystemPreferencesWindowClass,
      &base::win::WrappedWindowProc<SystemPreferences::WndProcStatic>, 0, 0, 0,
      NULL, NULL, NULL, NULL, NULL, &window_class);
  instance_ = window_class.hInstance;
  atom_ = RegisterClassEx(&window_class);

  // Create an offscreen window for receiving broadcast messages for the system
  // colorization color.  Create a hidden WS_POPUP window instead of an
  // HWND_MESSAGE window, because only top-level windows such as popups can
  // receive broadcast messages like "WM_DWMCOLORIZATIONCOLORCHANGED".
  window_ = CreateWindow(MAKEINTATOM(atom_), 0, WS_POPUP, 0, 0, 0, 0, 0, 0,
                         instance_, 0);
  gfx::CheckWindowCreated(window_, ::GetLastError());
  gfx::SetWindowUserData(window_, this);
}

LRESULT CALLBACK SystemPreferences::WndProcStatic(HWND hwnd,
                                                  UINT message,
                                                  WPARAM wparam,
                                                  LPARAM lparam) {
  auto* msg_wnd = reinterpret_cast<SystemPreferences*>(
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
    DWORD new_color = (DWORD)wparam;
    std::string new_color_string = hexColorDWORDToRGBA(new_color);
    if (new_color_string != current_color_) {
      Emit("accent-color-changed", hexColorDWORDToRGBA(new_color));
      current_color_ = new_color_string;
    }
  }
  return ::DefWindowProc(hwnd, message, wparam, lparam);
}

void SystemPreferences::OnSysColorChange() {
  bool new_inverted_color_scheme = IsInvertedColorScheme();
  if (new_inverted_color_scheme != inverted_color_scheme_) {
    inverted_color_scheme_ = new_inverted_color_scheme;
    Emit("inverted-color-scheme-changed", new_inverted_color_scheme);
  }

  bool new_high_contrast_color_scheme = IsHighContrastColorScheme();
  if (new_high_contrast_color_scheme != high_contrast_color_scheme_) {
    high_contrast_color_scheme_ = new_high_contrast_color_scheme;
    Emit("high-contrast-color-scheme-changed", new_high_contrast_color_scheme);
  }

  Emit("color-changed");
}

void SystemPreferences::OnFinishLaunching(base::Value::Dict launch_info) {
  color_change_listener_ =
      std::make_unique<gfx::ScopedSysColorChangeListener>(this);
}

}  // namespace api

}  // namespace electron
