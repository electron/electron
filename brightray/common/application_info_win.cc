#include <memory>

#include "common/application_info.h"

#include "base/file_version_info.h"
#include "base/strings/utf_string_conversions.h"

namespace brightray {

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

}  // namespace brightray
