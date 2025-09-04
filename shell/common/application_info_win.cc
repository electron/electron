// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/application_info.h"

#include <windows.h>  // windows.h must be included first

#include <VersionHelpers.h>
#include <appmodel.h>
#include <shlobj.h>

#include <memory>

#include "base/file_version_info.h"
#include "base/no_destructor.h"
#include "base/strings/string_util.h"
#include "base/strings/string_util_win.h"
#include "base/strings/utf_string_conversions.h"
#include "shell/browser/win/scoped_hstring.h"

namespace electron {

const wchar_t kAppUserModelIDFormat[] = L"electron.app.$1";

std::wstring& GetAppUserModelId() {
  static base::NoDestructor<std::wstring> g_app_user_model_id;
  return *g_app_user_model_id;
}

std::string GetApplicationName() {
  auto* module = GetModuleHandle(nullptr);
  std::unique_ptr<FileVersionInfo> info(
      FileVersionInfo::CreateFileVersionInfoForModule(module));
  return base::UTF16ToUTF8(info->product_name());
}

std::string GetApplicationVersion() {
  auto* module = GetModuleHandle(nullptr);
  std::unique_ptr<FileVersionInfo> info(
      FileVersionInfo::CreateFileVersionInfoForModule(module));
  return base::UTF16ToUTF8(info->product_version());
}

void SetAppUserModelID(const std::wstring& name) {
  GetAppUserModelId() = name;
  SetCurrentProcessExplicitAppUserModelID(GetAppUserModelId().c_str());
}

PCWSTR GetRawAppUserModelID() {
  if (GetAppUserModelId().empty()) {
    PWSTR current_app_id;
    if (SUCCEEDED(GetCurrentProcessExplicitAppUserModelID(&current_app_id))) {
      GetAppUserModelId() = current_app_id;
    } else {
      std::string name = GetApplicationName();
      std::wstring generated_app_id = base::ReplaceStringPlaceholders(
          kAppUserModelIDFormat, {base::UTF8ToWide(name)}, nullptr);
      SetAppUserModelID(generated_app_id);
    }
    CoTaskMemFree(current_app_id);
  }

  return GetAppUserModelId().c_str();
}

bool GetAppUserModelID(ScopedHString* app_id) {
  app_id->Reset(GetRawAppUserModelID());
  return app_id->success();
}

bool IsRunningInDesktopBridgeImpl() {
  UINT32 length = PACKAGE_FAMILY_NAME_MAX_LENGTH;
  wchar_t packageFamilyName[PACKAGE_FAMILY_NAME_MAX_LENGTH];
  HANDLE proc = GetCurrentProcess();
  LONG result = GetPackageFamilyName(proc, &length, packageFamilyName);
  return result == ERROR_SUCCESS;
}

bool IsRunningInDesktopBridge() {
  static bool result = IsRunningInDesktopBridgeImpl();
  return result;
}

}  // namespace electron
