#include "brightray/common/application_info.h"

#include <windows.h>  // windows.h must be included first

#include <shlobj.h>

#include <memory>

#include "base/file_version_info.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "brightray/browser/win/scoped_hstring.h"

namespace brightray {

namespace {

base::string16 app_user_model_id_;

}

const wchar_t kAppUserModelIDFormat[] = L"electron.app.$1";

std::string GetApplicationName() {
  auto module = GetModuleHandle(nullptr);
  std::unique_ptr<FileVersionInfo> info(
      FileVersionInfo::CreateFileVersionInfoForModule(module));
  return base::UTF16ToUTF8(info->product_name());
}

std::string GetApplicationVersion() {
  auto module = GetModuleHandle(nullptr);
  std::unique_ptr<FileVersionInfo> info(
      FileVersionInfo::CreateFileVersionInfoForModule(module));
  return base::UTF16ToUTF8(info->product_version());
}

void SetAppUserModelID(const base::string16& name) {
  app_user_model_id_ = name;
  SetCurrentProcessExplicitAppUserModelID(app_user_model_id_.c_str());
}

PCWSTR GetRawAppUserModelID() {
  if (app_user_model_id_.empty()) {
    PWSTR current_app_id;
    if (SUCCEEDED(GetCurrentProcessExplicitAppUserModelID(&current_app_id))) {
      app_user_model_id_ = current_app_id;
    } else {
      std::string name = GetOverridenApplicationName();
      if (name.empty()) {
        name = GetApplicationName();
      }
      base::string16 generated_app_id = base::ReplaceStringPlaceholders(
        kAppUserModelIDFormat, base::UTF8ToUTF16(name), nullptr);
      SetAppUserModelID(generated_app_id);
    }
    CoTaskMemFree(current_app_id);
  }

  return app_user_model_id_.c_str();
}

bool GetAppUserModelID(ScopedHString* app_id) {
  app_id->Reset(GetRawAppUserModelID());
  return app_id->success();
}

}  // namespace brightray
