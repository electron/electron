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
#include "electron/buildflags/buildflags.h"
#include "ppapi/shared_impl/ppapi_permissions.h"
#include "ui/base/l10n/l10n_util.h"
#include "url/url_constants.h"
// In SHARED_INTERMEDIATE_DIR.
#include "widevine_cdm_version.h"  // NOLINT(build/include)

#if defined(WIDEVINE_CDM_AVAILABLE)
#include "base/native_library.h"
#include "content/public/common/cdm_info.h"
#include "media/base/video_codecs.h"
#endif  // defined(WIDEVINE_CDM_AVAILABLE)

#if BUILDFLAG(ENABLE_PDF_VIEWER)
#include "atom/common/atom_constants.h"
#include "pdf/pdf.h"
#endif  // BUILDFLAG(ENABLE_PDF_VIEWER)

namespace atom {

namespace {

#if defined(WIDEVINE_CDM_AVAILABLE)
bool IsWidevineAvailable(
    base::FilePath* cdm_path,
    std::vector<media::VideoCodec>* codecs_supported,
    base::flat_set<media::CdmSessionType>* session_types_supported,
    base::flat_set<media::EncryptionMode>* modes_supported) {
  static enum {
    NOT_CHECKED,
    FOUND,
    NOT_FOUND,
  } widevine_cdm_file_check = NOT_CHECKED;

  if (widevine_cdm_file_check == NOT_CHECKED) {
    base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
    *cdm_path = command_line->GetSwitchValuePath(switches::kWidevineCdmPath);
    if (!cdm_path->empty()) {
      *cdm_path = cdm_path->AppendASCII(
          base::GetNativeLibraryName(kWidevineCdmLibraryName));
      widevine_cdm_file_check = base::PathExists(*cdm_path) ? FOUND : NOT_FOUND;
    }
  }

  if (widevine_cdm_file_check == FOUND) {
    // Add the supported codecs as if they came from the component manifest.
    // This list must match the CDM that is being bundled with Chrome.
    codecs_supported->push_back(media::VideoCodec::kCodecVP8);
    codecs_supported->push_back(media::VideoCodec::kCodecVP9);
#if BUILDFLAG(USE_PROPRIETARY_CODECS)
    codecs_supported->push_back(media::VideoCodec::kCodecH264);
#endif  // BUILDFLAG(USE_PROPRIETARY_CODECS)

    // TODO(crbug.com/767941): Push persistent-license support info here once
    // we check in a new CDM that supports it on Linux.
    session_types_supported->insert(media::CdmSessionType::kTemporary);
#if defined(OS_CHROMEOS)
    session_types_supported->insert(media::CdmSessionType::kPersistentLicense);
#endif  // defined(OS_CHROMEOS)

    modes_supported->insert(media::EncryptionMode::kCenc);

    return true;
  }

  return false;
}
#endif  // defined(WIDEVINE_CDM_AVAILABLE)

#if BUILDFLAG(ENABLE_PEPPER_FLASH)
content::PepperPluginInfo CreatePepperFlashInfo(const base::FilePath& path,
                                                const std::string& version) {
  content::PepperPluginInfo plugin;

  plugin.is_out_of_process = true;
  plugin.name = content::kFlashPluginName;
  plugin.path = path;
  plugin.permissions = ppapi::PERMISSION_ALL_BITS;

  std::vector<std::string> flash_version_numbers = base::SplitString(
      version, ".", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
  if (flash_version_numbers.empty())
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
                       flash_version_numbers[1] + " r" +
                       flash_version_numbers[2];
  plugin.version = base::JoinString(flash_version_numbers, ".");
  content::WebPluginMimeType swf_mime_type(content::kFlashPluginSwfMimeType,
                                           content::kFlashPluginSwfExtension,
                                           content::kFlashPluginSwfDescription);
  plugin.mime_types.push_back(swf_mime_type);
  content::WebPluginMimeType spl_mime_type(content::kFlashPluginSplMimeType,
                                           content::kFlashPluginSplExtension,
                                           content::kFlashPluginSplDescription);
  plugin.mime_types.push_back(spl_mime_type);

  return plugin;
}

void AddPepperFlashFromCommandLine(
    base::CommandLine* command_line,
    std::vector<content::PepperPluginInfo>* plugins) {
  base::FilePath flash_path =
      command_line->GetSwitchValuePath(switches::kPpapiFlashPath);
  if (flash_path.empty())
    return;

  auto flash_version =
      command_line->GetSwitchValueASCII(switches::kPpapiFlashVersion);

  plugins->push_back(CreatePepperFlashInfo(flash_path, flash_version));
}
#endif  // BUILDFLAG(ENABLE_PEPPER_FLASH)

void ComputeBuiltInPlugins(std::vector<content::PepperPluginInfo>* plugins) {
#if BUILDFLAG(ENABLE_PDF_VIEWER)
  content::PepperPluginInfo pdf_info;
  pdf_info.is_internal = true;
  pdf_info.is_out_of_process = true;
  pdf_info.name = "Chromium PDF Viewer";
  pdf_info.description = "Portable Document Format";
  pdf_info.path = base::FilePath::FromUTF8Unsafe(kPdfPluginPath);
  content::WebPluginMimeType pdf_mime_type(kPdfPluginMimeType, "pdf",
                                           "Portable Document Format");
  pdf_info.mime_types.push_back(pdf_mime_type);
  pdf_info.internal_entry_points.get_interface = chrome_pdf::PPP_GetInterface;
  pdf_info.internal_entry_points.initialize_module =
      chrome_pdf::PPP_InitializeModule;
  pdf_info.internal_entry_points.shutdown_module =
      chrome_pdf::PPP_ShutdownModule;
  pdf_info.permissions = ppapi::PERMISSION_PRIVATE | ppapi::PERMISSION_DEV;
  plugins->push_back(pdf_info);
#endif  // BUILDFLAG(ENABLE_PDF_VIEWER)
}

void ConvertStringWithSeparatorToVector(std::vector<std::string>* vec,
                                        const char* separator,
                                        const char* cmd_switch) {
  auto* command_line = base::CommandLine::ForCurrentProcess();
  auto string_with_separator = command_line->GetSwitchValueASCII(cmd_switch);
  if (!string_with_separator.empty())
    *vec = base::SplitString(string_with_separator, separator,
                             base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
}

}  // namespace

AtomContentClient::AtomContentClient() {}

AtomContentClient::~AtomContentClient() {}

std::string AtomContentClient::GetProduct() const {
  return "Chrome/" CHROME_VERSION_STRING;
}

std::string AtomContentClient::GetUserAgent() const {
  return content::BuildUserAgentFromProduct("Chrome/" CHROME_VERSION_STRING
                                            " " ATOM_PRODUCT_NAME
                                            "/" ATOM_VERSION_STRING);
}

base::string16 AtomContentClient::GetLocalizedString(int message_id) const {
  return l10n_util::GetStringUTF16(message_id);
}

void AtomContentClient::AddAdditionalSchemes(Schemes* schemes) {
  schemes->standard_schemes.push_back("chrome-extension");

  std::vector<std::string> splited;
  ConvertStringWithSeparatorToVector(&splited, ",",
                                     switches::kRegisterServiceWorkerSchemes);
  for (const std::string& scheme : splited)
    schemes->service_worker_schemes.push_back(scheme);
  schemes->service_worker_schemes.push_back(url::kFileScheme);

  ConvertStringWithSeparatorToVector(&splited, ",", switches::kSecureSchemes);
  for (const std::string& scheme : splited)
    schemes->secure_schemes.push_back(scheme);
}

void AtomContentClient::AddPepperPlugins(
    std::vector<content::PepperPluginInfo>* plugins) {
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
#if BUILDFLAG(ENABLE_PEPPER_FLASH)
  AddPepperFlashFromCommandLine(command_line, plugins);
#endif  // BUILDFLAG(ENABLE_PEPPER_FLASH)
  ComputeBuiltInPlugins(plugins);
}

void AtomContentClient::AddContentDecryptionModules(
    std::vector<content::CdmInfo>* cdms,
    std::vector<media::CdmHostFilePath>* cdm_host_file_paths) {
  if (cdms) {
#if defined(WIDEVINE_CDM_AVAILABLE)
    base::FilePath cdm_path;
    std::vector<media::VideoCodec> video_codecs_supported;
    base::flat_set<media::CdmSessionType> session_types_supported;
    base::flat_set<media::EncryptionMode> encryption_modes_supported;
    if (IsWidevineAvailable(&cdm_path, &video_codecs_supported,
                            &session_types_supported,
                            &encryption_modes_supported)) {
      base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
      auto cdm_version_string =
          command_line->GetSwitchValueASCII(switches::kWidevineCdmVersion);
      // CdmInfo needs |path| to be the actual Widevine library,
      // not the adapter, so adjust as necessary. It will be in the
      // same directory as the installed adapter.
      const base::Version version(cdm_version_string);
      DCHECK(version.IsValid());

      content::CdmCapability capability(
          video_codecs_supported, encryption_modes_supported,
          session_types_supported, base::flat_set<media::CdmProxy::Protocol>());

      cdms->push_back(content::CdmInfo(
          kWidevineCdmDisplayName, kWidevineCdmGuid, version, cdm_path,
          kWidevineCdmFileSystemId, capability, kWidevineKeySystem, false));
    }
#endif  // defined(WIDEVINE_CDM_AVAILABLE)
  }
}

}  // namespace atom
