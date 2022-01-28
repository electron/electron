// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/app/electron_content_client.h"

#include <string>
#include <utility>
#include <vector>

#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "content/public/common/content_constants.h"
#include "content/public/common/content_switches.h"
#include "electron/buildflags/buildflags.h"
#include "extensions/common/constants.h"
#include "ppapi/buildflags/buildflags.h"
#include "shell/common/electron_paths.h"
#include "shell/common/options_switches.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "url/url_constants.h"
// In SHARED_INTERMEDIATE_DIR.
#include "widevine_cdm_version.h"  // NOLINT(build/include_directory)

#if defined(WIDEVINE_CDM_AVAILABLE)
#include "base/native_library.h"
#include "content/public/common/cdm_info.h"
#include "media/base/video_codecs.h"
#endif  // defined(WIDEVINE_CDM_AVAILABLE)

#if BUILDFLAG(ENABLE_PDF_VIEWER)
#include "pdf/pdf.h"        // nogncheck
#include "pdf/pdf_ppapi.h"  // nogncheck
#include "shell/common/electron_constants.h"
#endif  // BUILDFLAG(ENABLE_PDF_VIEWER)

#if BUILDFLAG(ENABLE_PLUGINS)
#include "content/public/browser/plugin_service.h"
#include "content/public/common/pepper_plugin_info.h"
#include "ppapi/shared_impl/ppapi_permissions.h"
#endif  // BUILDFLAG(ENABLE_PLUGINS)

namespace electron {

namespace {

enum class WidevineCdmFileCheck {
  kNotChecked,
  kFound,
  kNotFound,
};

#if defined(WIDEVINE_CDM_AVAILABLE)
bool IsWidevineAvailable(
    base::FilePath* cdm_path,
    std::vector<media::VideoCodec>* codecs_supported,
    base::flat_set<media::CdmSessionType>* session_types_supported,
    base::flat_set<media::EncryptionMode>* modes_supported) {
  static WidevineCdmFileCheck widevine_cdm_file_check =
      WidevineCdmFileCheck::kNotChecked;

  if (widevine_cdm_file_check == WidevineCdmFileCheck::kNotChecked) {
    base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
    *cdm_path = command_line->GetSwitchValuePath(switches::kWidevineCdmPath);
    if (!cdm_path->empty()) {
      *cdm_path = cdm_path->AppendASCII(
          base::GetNativeLibraryName(kWidevineCdmLibraryName));
      widevine_cdm_file_check = base::PathExists(*cdm_path)
                                    ? WidevineCdmFileCheck::kFound
                                    : WidevineCdmFileCheck::kNotFound;
    }
  }

  if (widevine_cdm_file_check == WidevineCdmFileCheck::kFound) {
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
#if BUILDFLAG(IS_CHROMEOS)
    session_types_supported->insert(media::CdmSessionType::kPersistentLicense);
#endif  // BUILDFLAG(IS_CHROMEOS)

    modes_supported->insert(media::EncryptionMode::kCenc);

    return true;
  }

  return false;
}
#endif  // defined(WIDEVINE_CDM_AVAILABLE)

#if BUILDFLAG(ENABLE_PLUGINS)
void ComputeBuiltInPlugins(std::vector<content::PepperPluginInfo>* plugins) {
#if BUILDFLAG(ENABLE_PDF_VIEWER)
  content::PepperPluginInfo pdf_info;
  pdf_info.is_internal = true;
  pdf_info.is_out_of_process = true;
  pdf_info.name = "Chromium PDF Viewer";
  pdf_info.description = "Portable Document Format";
  // This isn't a real file path; it's just used as a unique identifier.
  pdf_info.path = base::FilePath(kPdfPluginPath);
  content::WebPluginMimeType pdf_mime_type(kPdfPluginMimeType, "pdf",
                                           "Portable Document Format");
  pdf_info.mime_types.push_back(pdf_mime_type);
  pdf_info.internal_entry_points.get_interface = chrome_pdf::PPP_GetInterface;
  pdf_info.internal_entry_points.initialize_module =
      chrome_pdf::PPP_InitializeModule;
  pdf_info.internal_entry_points.shutdown_module =
      chrome_pdf::PPP_ShutdownModule;
  pdf_info.permissions = ppapi::PERMISSION_PDF | ppapi::PERMISSION_DEV;
  plugins->push_back(pdf_info);

  // NB. in Chrome, this plugin isn't registered until the PDF extension is
  // loaded. However, in Electron, we load the PDF extension unconditionally
  // when it is enabled in the build, so we're OK to load the plugin eagerly
  // here.
  content::WebPluginInfo info;
  info.type = content::WebPluginInfo::PLUGIN_TYPE_BROWSER_PLUGIN;
  info.name = u"Chromium PDF Viewer";
  // This isn't a real file path; it's just used as a unique identifier.
  info.path = base::FilePath::FromUTF8Unsafe(extension_misc::kPdfExtensionId);
  info.background_color = content::WebPluginInfo::kDefaultBackgroundColor;
  info.mime_types.emplace_back("application/pdf", "pdf",
                               "Portable Document Format");
  content::PluginService::GetInstance()->RefreshPlugins();
  content::PluginService::GetInstance()->RegisterInternalPlugin(info, true);
#endif  // BUILDFLAG(ENABLE_PDF_VIEWER)
}
#endif  // BUILDFLAG(ENABLE_PLUGINS)

void AppendDelimitedSwitchToVector(const base::StringPiece cmd_switch,
                                   std::vector<std::string>* append_me) {
  auto* command_line = base::CommandLine::ForCurrentProcess();
  auto switch_value = command_line->GetSwitchValueASCII(cmd_switch);
  if (!switch_value.empty()) {
    constexpr base::StringPiece delimiter(",", 1);
    auto tokens =
        base::SplitString(switch_value, delimiter, base::TRIM_WHITESPACE,
                          base::SPLIT_WANT_NONEMPTY);
    append_me->reserve(append_me->size() + tokens.size());
    std::move(std::begin(tokens), std::end(tokens),
              std::back_inserter(*append_me));
  }
}

}  // namespace

ElectronContentClient::ElectronContentClient() = default;

ElectronContentClient::~ElectronContentClient() = default;

std::u16string ElectronContentClient::GetLocalizedString(int message_id) {
  return l10n_util::GetStringUTF16(message_id);
}

base::StringPiece ElectronContentClient::GetDataResource(
    int resource_id,
    ui::ResourceScaleFactor scale_factor) {
  return ui::ResourceBundle::GetSharedInstance().GetRawDataResourceForScale(
      resource_id, scale_factor);
}

gfx::Image& ElectronContentClient::GetNativeImageNamed(int resource_id) {
  return ui::ResourceBundle::GetSharedInstance().GetNativeImageNamed(
      resource_id);
}

base::RefCountedMemory* ElectronContentClient::GetDataResourceBytes(
    int resource_id) {
  return ui::ResourceBundle::GetSharedInstance().LoadDataResourceBytes(
      resource_id);
}

void ElectronContentClient::AddAdditionalSchemes(Schemes* schemes) {
  auto* command_line = base::CommandLine::ForCurrentProcess();
  std::string process_type =
      command_line->GetSwitchValueASCII(::switches::kProcessType);
  // Browser Process registration happens in
  // `api::Protocol::RegisterSchemesAsPrivileged`
  //
  // Renderer Process registration happens in `RendererClientBase`
  //
  // We use this for registration to network utility process
  if (process_type == ::switches::kUtilityProcess) {
    AppendDelimitedSwitchToVector(switches::kServiceWorkerSchemes,
                                  &schemes->service_worker_schemes);
    AppendDelimitedSwitchToVector(switches::kStandardSchemes,
                                  &schemes->standard_schemes);
    AppendDelimitedSwitchToVector(switches::kSecureSchemes,
                                  &schemes->secure_schemes);
    AppendDelimitedSwitchToVector(switches::kBypassCSPSchemes,
                                  &schemes->csp_bypassing_schemes);
    AppendDelimitedSwitchToVector(switches::kCORSSchemes,
                                  &schemes->cors_enabled_schemes);
  }

  schemes->service_worker_schemes.emplace_back(url::kFileScheme);

#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
  schemes->standard_schemes.push_back(extensions::kExtensionScheme);
  schemes->savable_schemes.push_back(extensions::kExtensionScheme);
  schemes->secure_schemes.push_back(extensions::kExtensionScheme);
  schemes->service_worker_schemes.push_back(extensions::kExtensionScheme);
  schemes->cors_enabled_schemes.push_back(extensions::kExtensionScheme);
  schemes->csp_bypassing_schemes.push_back(extensions::kExtensionScheme);
#endif
}

void ElectronContentClient::AddPepperPlugins(
    std::vector<content::PepperPluginInfo>* plugins) {
#if BUILDFLAG(ENABLE_PLUGINS)
  ComputeBuiltInPlugins(plugins);
#endif  // BUILDFLAG(ENABLE_PLUGINS)
}

void ElectronContentClient::AddContentDecryptionModules(
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

}  // namespace electron
