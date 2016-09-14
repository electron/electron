// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/pepper_flash_util.h"

#include "atom/common/options_switches.h"
#include "base/command_line.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "content/public/common/content_constants.h"
#include "ppapi/shared_impl/ppapi_permissions.h"

namespace atom {

content::PepperPluginInfo CreatePepperFlashInfo(const base::FilePath& path,
                                                const std::string& version) {
  content::PepperPluginInfo plugin;

  plugin.is_out_of_process = true;
  plugin.name = content::kFlashPluginName;
  plugin.path = path;
#if defined(OS_WIN)
  plugin.is_on_local_drive = !base::IsOnNetworkDrive(path);
#endif
  plugin.permissions = ppapi::PERMISSION_DEV | ppapi::PERMISSION_PRIVATE |
    ppapi::PERMISSION_BYPASS_USER_GESTURE | ppapi::PERMISSION_FLASH;
  plugin.is_external = true;

  std::vector<std::string> flash_version_numbers = base::SplitString(
      version, ".", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
  if (flash_version_numbers.size() < 1)
    flash_version_numbers.push_back("11");
  // |SplitString()| puts in an empty string given an empty string. :(
  else if (flash_version_numbers[0].empty())
    flash_version_numbers[0] = "11";
  if (flash_version_numbers.size() < 2)
    flash_version_numbers.push_back("2");
  if (flash_version_numbers.size() < 3)
    flash_version_numbers.push_back("999");
  if (flash_version_numbers.size() < 4)
    flash_version_numbers.push_back("999");
  // E.g., "Shockwave Flash 10.2 r154":
  plugin.description = plugin.name + " " + flash_version_numbers[0] + "." +
      flash_version_numbers[1] + " r" + flash_version_numbers[2];
  plugin.version = base::JoinString(flash_version_numbers, ".");
  content::WebPluginMimeType swf_mime_type(
      content::kFlashPluginSwfMimeType,
      content::kFlashPluginSwfExtension,
      content::kFlashPluginSwfDescription);
  plugin.mime_types.push_back(swf_mime_type);
  content::WebPluginMimeType spl_mime_type(
      content::kFlashPluginSplMimeType,
      content::kFlashPluginSplExtension,
      content::kFlashPluginSplDescription);
  plugin.mime_types.push_back(spl_mime_type);

  return plugin;
}

void AddPepperFlashFromCommandLine(
    std::vector<content::PepperPluginInfo>* plugins) {
  auto command_line = base::CommandLine::ForCurrentProcess();
  const base::CommandLine::StringType flash_path =
    command_line->GetSwitchValueNative(switches::kPpapiFlashPath);
  if (flash_path.empty())
    return;

  auto flash_version = command_line->GetSwitchValueNative(
      switches::kPpapiFlashVersion);

  plugins->push_back(
      CreatePepperFlashInfo(base::FilePath(flash_path), flash_version));
}

}  // namespace atom
