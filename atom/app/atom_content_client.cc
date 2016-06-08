// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/app/atom_content_client.h"

#include <string>
#include <vector>

#include "atom/common/atom_version.h"
#include "atom/common/chrome_version.h"
#include "atom/common/options_switches.h"
#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "content/public/common/content_constants.h"
#include "content/public/common/pepper_plugin_info.h"
#include "content/public/common/user_agent.h"
#include "ppapi/shared_impl/ppapi_permissions.h"
#include "third_party/widevine/cdm/stub/widevine_cdm_version.h"
#include "ui/base/l10n/l10n_util.h"
#include "url/url_constants.h"

#if defined(WIDEVINE_CDM_AVAILABLE) && defined(ENABLE_PEPPER_CDMS)
#include "chrome/common/widevine_cdm_constants.h"
#endif

namespace atom {

namespace {

content::PepperPluginInfo CreatePepperFlashInfo(const base::FilePath& path,
                                                const std::string& version) {
  content::PepperPluginInfo plugin;

  plugin.is_out_of_process = true;
  plugin.name = content::kFlashPluginName;
  plugin.path = path;
  plugin.permissions = ppapi::PERMISSION_ALL_BITS;

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

#if defined(WIDEVINE_CDM_AVAILABLE) && defined(ENABLE_PEPPER_CDMS)
content::PepperPluginInfo CreateWidevineCdmInfo(const base::FilePath& path,
                                                const std::string& version) {
  content::PepperPluginInfo widevine_cdm;
  widevine_cdm.is_out_of_process = true;
  widevine_cdm.path = path;
  widevine_cdm.name = kWidevineCdmDisplayName;
  widevine_cdm.description = kWidevineCdmDescription +
                             std::string(" (version: ") +
                             version + ")";
  widevine_cdm.version = version;
  content::WebPluginMimeType widevine_cdm_mime_type(
      kWidevineCdmPluginMimeType,
      kWidevineCdmPluginExtension,
      kWidevineCdmPluginMimeTypeDescription);

  // Add the supported codecs as if they came from the component manifest.
  std::vector<std::string> codecs;
  codecs.push_back(kCdmSupportedCodecVp8);
  codecs.push_back(kCdmSupportedCodecVp9);
#if defined(USE_PROPRIETARY_CODECS)
  codecs.push_back(kCdmSupportedCodecAvc1);
#endif  // defined(USE_PROPRIETARY_CODECS)
  std::string codec_string = base::JoinString(
      codecs, std::string(1, kCdmSupportedCodecsValueDelimiter));
  widevine_cdm_mime_type.additional_param_names.push_back(
      base::ASCIIToUTF16(kCdmSupportedCodecsParamName));
  widevine_cdm_mime_type.additional_param_values.push_back(
      base::ASCIIToUTF16(codec_string));

  widevine_cdm.mime_types.push_back(widevine_cdm_mime_type);
  widevine_cdm.permissions = kWidevineCdmPluginPermissions;

  return widevine_cdm;
}
#endif

void ConvertStringWithSeparatorToVector(std::vector<std::string>* vec,
                                        const char* separator,
                                        const char* cmd_switch) {
  auto command_line = base::CommandLine::ForCurrentProcess();
  auto string_with_separator = command_line->GetSwitchValueASCII(cmd_switch);
  if (!string_with_separator.empty())
    *vec = base::SplitString(string_with_separator, separator,
                             base::TRIM_WHITESPACE,
                             base::SPLIT_WANT_NONEMPTY);
}

}  // namespace

void AddPepperFlashFromCommandLine(
    std::vector<content::PepperPluginInfo>* plugins) {
  auto command_line = base::CommandLine::ForCurrentProcess();
  base::FilePath flash_path = command_line->GetSwitchValuePath(
      switches::kPpapiFlashPath);
  if (flash_path.empty())
    return;

  auto flash_version = command_line->GetSwitchValueASCII(
      switches::kPpapiFlashVersion);

  plugins->push_back(CreatePepperFlashInfo(flash_path, flash_version));
}

#if defined(WIDEVINE_CDM_AVAILABLE) && defined(ENABLE_PEPPER_CDMS)
void AddWidevineCdmFromCommandLine(
    std::vector<content::PepperPluginInfo>* plugins) {
  auto command_line = base::CommandLine::ForCurrentProcess();
  base::FilePath widevine_cdm_path = command_line->GetSwitchValuePath(
      switches::kWidevineCdmPath);
  if (widevine_cdm_path.empty())
    return;

  if (!base::PathExists(widevine_cdm_path))
    return;

  auto widevine_cdm_version = command_line->GetSwitchValueASCII(
      switches::kWidevineCdmVersion);
  if (widevine_cdm_version.empty())
    return;

  plugins->push_back(CreateWidevineCdmInfo(widevine_cdm_path,
                                           widevine_cdm_version));
}
#endif

AtomContentClient::AtomContentClient() {
}

AtomContentClient::~AtomContentClient() {
}

std::string AtomContentClient::GetProduct() const {
  return "Chrome/" CHROME_VERSION_STRING;
}

std::string AtomContentClient::GetUserAgent() const {
  return content::BuildUserAgentFromProduct(
      "Chrome/" CHROME_VERSION_STRING " "
      ATOM_PRODUCT_NAME "/" ATOM_VERSION_STRING);
}

base::string16 AtomContentClient::GetLocalizedString(int message_id) const {
  return l10n_util::GetStringUTF16(message_id);
}

void AtomContentClient::AddAdditionalSchemes(
    std::vector<url::SchemeWithType>* standard_schemes,
    std::vector<url::SchemeWithType>* referrer_schemes,
    std::vector<std::string>* savable_schemes) {
  // Parse --standard-schemes=scheme1,scheme2
  auto command_line = base::CommandLine::ForCurrentProcess();
  std::string custom_schemes = command_line->GetSwitchValueASCII(
      switches::kStandardSchemes);
  if (!custom_schemes.empty()) {
    // Note that url::SchemeWithType requires passing const char*, so we have
    // to ensure the string still lives after this function exits.
    static std::vector<std::string> schemes_list;
    schemes_list = base::SplitString(
        custom_schemes, ",", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
    for (const std::string& scheme : schemes_list)
      standard_schemes->push_back({scheme.c_str(), url::SCHEME_WITHOUT_PORT});
  }

  standard_schemes->push_back({"chrome-extension", url::SCHEME_WITHOUT_PORT});
}

void AtomContentClient::AddPepperPlugins(
    std::vector<content::PepperPluginInfo>* plugins) {
  AddPepperFlashFromCommandLine(plugins);
#if defined(WIDEVINE_CDM_AVAILABLE) && defined(ENABLE_PEPPER_CDMS)
  AddWidevineCdmFromCommandLine(plugins);
#endif
}

void AtomContentClient::AddServiceWorkerSchemes(
    std::set<std::string>* service_worker_schemes) {
  std::vector<std::string> schemes;
  ConvertStringWithSeparatorToVector(&schemes, ",",
                                     switches::kRegisterServiceWorkerSchemes);
  if (!schemes.empty()) {
    for (const std::string& scheme : schemes)
      service_worker_schemes->insert(scheme);
  }
  service_worker_schemes->insert(url::kFileScheme);
}

}  // namespace atom
