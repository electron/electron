#include "brightray/common/application_info.h"

#include <windows.h>  // windows.h must be included first

#include <appmodel.h>
#include <shlobj.h>

#include <memory>

#include "base/file_version_info.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "brightray/browser/win/scoped_hstring.h"

namespace brightray {

namespace {

base::string16 g_app_user_model_id;
}

const wchar_t kAppUserModelIDFormat[] = L"electron.app.$1";
bool hasCheckedIsRunningInDesktopBridge = false;
bool isRunningInDesktopBridge = false;

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

void SetAppUserModelID(const base::string16& name) {
  g_app_user_model_id = name;
  SetCurrentProcessExplicitAppUserModelID(g_app_user_model_id.c_str());
}

PCWSTR GetRawAppUserModelID() {
  if (g_app_user_model_id.empty()) {
    PWSTR current_app_id;
    if (SUCCEEDED(GetCurrentProcessExplicitAppUserModelID(&current_app_id))) {
      g_app_user_model_id = current_app_id;
    } else {
      std::string name = GetApplicationName();
      base::string16 generated_app_id = base::ReplaceStringPlaceholders(
          kAppUserModelIDFormat, base::UTF8ToUTF16(name), nullptr);
      SetAppUserModelID(generated_app_id);
    }
    CoTaskMemFree(current_app_id);
  }

  return g_app_user_model_id.c_str();
}

bool GetAppUserModelID(ScopedHString* app_id) {
  app_id->Reset(GetRawAppUserModelID());
  return app_id->success();
}

bool IsRunningInDesktopBridge() {
  if (hasCheckedIsRunningInDesktopBridge) {
    return isRunningInDesktopBridge;
  }

  UINT32 length;
  wchar_t packageFamilyName[PACKAGE_FAMILY_NAME_MAX_LENGTH + 1];
  HANDLE proc = GetCurrentProcess();
  LONG result = GetPackageFamilyName(proc, &length, packageFamilyName);

  isRunningInDesktopBridge = result == ERROR_SUCCESS;
  hasCheckedIsRunningInDesktopBridge = true;

  return isRunningInDesktopBridge;
}

}  // namespace brightray
