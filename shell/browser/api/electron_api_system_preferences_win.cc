// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <string_view>

#include <windows.devices.enumeration.h>
#include <wrl/client.h>

#include "shell/browser/api/electron_api_system_preferences.h"

#include "base/containers/fixed_flat_map.h"
#include "base/logging.h"
#include "base/win/core_winrt_util.h"
#include "base/win/windows_types.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "gin/persistent.h"
#include "shell/browser/javascript_environment.h"
#include "shell/common/color_util.h"
#include "shell/common/process_util.h"
#include "ui/color/win/accent_color_observer.h"
#include "ui/gfx/win/singleton_hwnd.h"
#include "v8/include/v8-cppgc.h"

namespace electron {

namespace {

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

std::string SystemPreferences::GetAccentColor() {
  const std::optional<SkColor> color =
      ui::AccentColorObserver::Get()->accent_color();
  if (!color.has_value())
    return "";

  return ToRGBAHex(*color, false);
}

void SystemPreferences::OnSystemAccentColorChanged() {
  // Compare and emit from a fresh task rather than from inside the observer's
  // notification, which runs app JS before the observer re-arms its registry
  // watch.
  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  content::GetUIThreadTaskRunner({})->PostTask(
      FROM_HERE,
      base::BindOnce(&SystemPreferences::OnAccentColorChanged,
                     gin::WrapPersistent(weak_factory_.GetWeakCell(
                         isolate->GetCppHeap()->GetAllocationHandle()))));
}

void SystemPreferences::OnAccentColorChanged() {
  std::string new_color = GetAccentColor();
  if (new_color == current_color_)
    return;
  current_color_ = new_color;
  Emit("accent-color-changed", new_color);
}

std::string SystemPreferences::GetColor(gin_helper::ErrorThrower thrower,
                                        const std::string& color) {
  static constexpr auto Lookup = base::MakeFixedFlatMap<std::string_view, int>({
      {"3d-dark-shadow", COLOR_3DDKSHADOW},
      {"3d-face", COLOR_3DFACE},
      {"3d-highlight", COLOR_3DHIGHLIGHT},
      {"3d-light", COLOR_3DLIGHT},
      {"3d-shadow", COLOR_3DSHADOW},
      {"active-border", COLOR_ACTIVEBORDER},
      {"active-caption", COLOR_ACTIVECAPTION},
      {"active-caption-gradient", COLOR_GRADIENTACTIVECAPTION},
      {"app-workspace", COLOR_APPWORKSPACE},
      {"button-text", COLOR_BTNTEXT},
      {"caption-text", COLOR_CAPTIONTEXT},
      {"desktop", COLOR_DESKTOP},
      {"disabled-text", COLOR_GRAYTEXT},
      {"highlight", COLOR_HIGHLIGHT},
      {"highlight-text", COLOR_HIGHLIGHTTEXT},
      {"hotlight", COLOR_HOTLIGHT},
      {"inactive-border", COLOR_INACTIVEBORDER},
      {"inactive-caption", COLOR_INACTIVECAPTION},
      {"inactive-caption-gradient", COLOR_GRADIENTINACTIVECAPTION},
      {"inactive-caption-text", COLOR_INACTIVECAPTIONTEXT},
      {"info-background", COLOR_INFOBK},
      {"info-text", COLOR_INFOTEXT},
      {"menu", COLOR_MENU},
      {"menu-highlight", COLOR_MENUHILIGHT},
      {"menu-text", COLOR_MENUTEXT},
      {"menubar", COLOR_MENUBAR},
      {"scrollbar", COLOR_SCROLLBAR},
      {"window", COLOR_WINDOW},
      {"window-frame", COLOR_WINDOWFRAME},
      {"window-text", COLOR_WINDOWTEXT},
  });

  if (auto iter = Lookup.find(color); iter != Lookup.end())
    return ToRGBAHex(GetSysSkColor(iter->second));

  thrower.ThrowError("Unknown color: " + color);
  return "";
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
    return {};
  }
}

void SystemPreferences::InitializeWindow() {
  if (electron::IsUtilityProcess())
    return;
  // Wait until app is ready before creating sys color listener
  // Creating this listener before the app is ready causes global shortcuts
  // to not fire
  if (Browser::Get()->is_ready()) {
    hwnd_subscription_ =
        gfx::SingletonHwnd::GetInstance()->RegisterCallback(base::BindRepeating(
            &SystemPreferences::OnWndProc, base::Unretained(this)));
  } else {
    Browser::Get()->AddObserver(this);
  }

  current_color_ = GetAccentColor();
  accent_color_subscription_ = ui::AccentColorObserver::Get()->Subscribe(
      base::BindRepeating(&SystemPreferences::OnSystemAccentColorChanged,
                          base::Unretained(this)));
}

void SystemPreferences::OnWndProc(HWND hwnd,
                                  UINT message,
                                  WPARAM wparam,
                                  LPARAM lparam) {
  if (message != WM_SYSCOLORCHANGE &&
      (message != WM_SETTINGCHANGE || wparam != SPI_SETHIGHCONTRAST)) {
    return;
  }
  Emit("color-changed");
}

void SystemPreferences::OnFinishLaunching(base::DictValue launch_info) {
  hwnd_subscription_ =
      gfx::SingletonHwnd::GetInstance()->RegisterCallback(base::BindRepeating(
          &SystemPreferences::OnWndProc, base::Unretained(this)));
  Browser::Get()->RemoveObserver(this);
}

void SystemPreferences::Dispose() {
  if (electron::IsUtilityProcess())
    return;

  hwnd_subscription_ = {};
  accent_color_subscription_ = {};
  weak_factory_.Invalidate();
  Browser::Get()->RemoveObserver(this);
}

}  // namespace api

}  // namespace electron
