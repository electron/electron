#include "common/application_info.h"

#include "base/file_version_info.h"
#include "base/memory/scoped_ptr.h"
#include "base/strings/utf_string_conversions.h"

namespace brightray {

std::string GetApplicationName() {
  auto info = make_scoped_ptr(FileVersionInfo::CreateFileVersionInfoForModule(GetModuleHandle(nullptr)));
  return UTF16ToUTF8(info->product_name());
}

std::string GetApplicationVersion() {
  auto info = make_scoped_ptr(FileVersionInfo::CreateFileVersionInfoForModule(GetModuleHandle(nullptr)));
  return UTF16ToUTF8(info->product_version());
}

}
