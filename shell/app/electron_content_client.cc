// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/app/electron_content_client.h"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "base/command_line.h"
#include "base/containers/extend.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/no_destructor.h"
#include "base/path_service.h"
#include "base/strings/string_split.h"
#include "build/build_config.h"
#include "content/public/common/buildflags.h"
#include "electron/buildflags/buildflags.h"
#include "electron/fuses.h"
#include "extensions/common/constants.h"
#include "pdf/buildflags.h"
#include "shell/common/options_switches.h"
#include "shell/common/process_util.h"
#include "third_party/widevine/cdm/buildflags.h"
#include "third_party/widevine/cdm/widevine_cdm_common.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "url/url_constants.h"

#if BUILDFLAG(ENABLE_WIDEVINE)
#include "base/json/json_file_value_serializer.h"
#include "base/native_library.h"
#include "components/cdm/common/cdm_manifest.h"
#include "content/public/common/cdm_info.h"
#include "media/base/audio_codecs.h"
#include "media/base/cdm_capability.h"
#include "media/base/video_codecs.h"
#include "media/cdm/cdm_paths.h"
#endif  // BUILDFLAG(ENABLE_WIDEVINE)

#if BUILDFLAG(ENABLE_PDF_VIEWER)
#include "components/pdf/common/constants.h"  // nogncheck
#include "shell/common/electron_constants.h"
#endif  // BUILDFLAG(ENABLE_PDF_VIEWER)

#if BUILDFLAG(ENABLE_PLUGINS)
#include "content/public/common/webplugininfo.h"
#endif  // BUILDFLAG(ENABLE_PLUGINS)

namespace electron {

namespace {

enum class WidevineCdmFileCheck {
  kNotChecked,
  kFound,
  kNotFound,
};

#if BUILDFLAG(ENABLE_WIDEVINE)
std::optional<base::FilePath> GetWidevineSubPackagePath(
    const base::Value::Dict& manifest) {
  const base::Value::List* platforms = manifest.FindList("platforms");
  if (!platforms)
    return std::nullopt;

  base::FilePath platform_specific_dir =
      media::GetPlatformSpecificDirectory(base::FilePath());
  if (platform_specific_dir.empty())
    return std::nullopt;
  platform_specific_dir = platform_specific_dir.StripTrailingSeparators();

  for (const auto& entry : *platforms) {
    if (!entry.is_dict())
      continue;
    const base::Value::Dict& dict = entry.GetDict();
    const std::string* sub_package_path = dict.FindString("sub_package_path");
    if (!sub_package_path)
      continue;
    base::FilePath candidate = base::FilePath::FromUTF8Unsafe(*sub_package_path)
                                   .NormalizePathSeparators()
                                   .StripTrailingSeparators();
    if (candidate == platform_specific_dir) {
      return base::FilePath::FromUTF8Unsafe(*sub_package_path);
    }
  }

  return std::nullopt;
}

bool ParseWidevineManifestFromPath(
    const base::FilePath& manifest_path,
    media::CdmCapability* capability,
    std::optional<base::FilePath>* sub_package_path_out) {
  JSONFileValueDeserializer deserializer(manifest_path);
  int error_code;
  std::string error_message;
  std::unique_ptr<base::Value> manifest =
      deserializer.Deserialize(&error_code, &error_message);
  if (!manifest || !manifest->is_dict())
    return false;

  base::Value::Dict& manifest_dict = manifest->GetDict();
  if (!IsCdmManifestCompatibleWithChrome(manifest_dict) ||
      !ParseCdmManifest(manifest_dict, capability)) {
    return false;
  }

  if (sub_package_path_out)
    *sub_package_path_out = GetWidevineSubPackagePath(manifest_dict);
  return true;
}

bool TryGetBundledWidevineCdm(
    base::FilePath* cdm_path,
    std::optional<media::CdmCapability>* capability_out) {
  base::FilePath exe_dir;
  if (!base::PathService::Get(base::DIR_EXE, &exe_dir))
    return false;

  base::FilePath install_dir = exe_dir.AppendASCII(kWidevineCdmBaseDirectory);
  base::FilePath platform_dir =
      media::GetPlatformSpecificDirectory(install_dir);
  base::FilePath cdm_library_path =
      platform_dir.Append(base::GetNativeLibraryName(kWidevineCdmLibraryName));
  if (!base::PathExists(cdm_library_path))
    return false;

  base::FilePath manifest_path =
      install_dir.Append(FILE_PATH_LITERAL("manifest.json"));
  media::CdmCapability capability;
  if (!ParseCdmManifestFromPath(manifest_path, &capability))
    return false;

  *cdm_path = cdm_library_path;
  *capability_out = std::move(capability);
  return true;
}

std::optional<base::Version> GetWidevineVersionFromSwitch() {
  const std::string version_string =
      base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
          switches::kWidevineCdmVersion);
  base::Version parsed(version_string);
  if (parsed.IsValid()) {
    return parsed;
  }
  return std::nullopt;
}

bool IsWidevineAvailable(
    base::FilePath* cdm_path,
    std::vector<media::AudioCodec>* audio_codecs_supported,
    std::vector<media::VideoCodec>* codecs_supported,
    base::flat_set<media::CdmSessionType>* session_types_supported,
    base::flat_set<media::EncryptionScheme>* modes_supported,
    std::optional<base::Version>* cdm_version_out) {
  static WidevineCdmFileCheck widevine_cdm_file_check =
      WidevineCdmFileCheck::kNotChecked;
  static base::NoDestructor<std::optional<media::CdmCapability>>
      cached_capability;

  if (widevine_cdm_file_check == WidevineCdmFileCheck::kNotChecked) {
    base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
    *cdm_path = command_line->GetSwitchValuePath(switches::kWidevineCdmPath);
    if (cdm_path->empty()) {
      if (TryGetBundledWidevineCdm(cdm_path, cached_capability.get())) {
        widevine_cdm_file_check = WidevineCdmFileCheck::kFound;
        if (cdm_version_out)
          *cdm_version_out = (*cached_capability)->version;
      } else {
        widevine_cdm_file_check = WidevineCdmFileCheck::kNotFound;
      }
    } else {
      base::FilePath cdm_dir = *cdm_path;
      base::FilePath manifest_path =
          cdm_dir.Append(FILE_PATH_LITERAL("manifest.json"));
      media::CdmCapability capability;
      std::optional<base::FilePath> sub_package_path;
      if (ParseWidevineManifestFromPath(manifest_path, &capability,
                                        &sub_package_path)) {
        *cached_capability = std::move(capability);
        if (cdm_version_out)
          *cdm_version_out = (*cached_capability)->version;
      } else if (cdm_version_out) {
        *cdm_version_out = GetWidevineVersionFromSwitch();
      }

      base::FilePath cdm_library_dir = cdm_dir;
      if (sub_package_path && !sub_package_path->empty()) {
        if (sub_package_path->IsAbsolute()) {
          cdm_library_dir = *sub_package_path;
        } else {
          cdm_library_dir = cdm_dir.Append(*sub_package_path);
        }
      }

      *cdm_path = cdm_library_dir.AppendASCII(
          base::GetNativeLibraryName(kWidevineCdmLibraryName));
      widevine_cdm_file_check = base::PathExists(*cdm_path)
                                    ? WidevineCdmFileCheck::kFound
                                    : WidevineCdmFileCheck::kNotFound;
      if (!cached_capability->has_value())
        cached_capability->reset();
    }
  }

  if (widevine_cdm_file_check == WidevineCdmFileCheck::kFound) {
    if (cached_capability->has_value()) {
      for (auto codec : (*cached_capability)->audio_codecs) {
        audio_codecs_supported->push_back(codec);
      }
      for (const auto& entry : (*cached_capability)->video_codecs) {
        codecs_supported->push_back(entry.first);
      }
      session_types_supported->insert(
          (*cached_capability)->session_types.begin(),
          (*cached_capability)->session_types.end());
      modes_supported->insert((*cached_capability)->encryption_schemes.begin(),
                              (*cached_capability)->encryption_schemes.end());
    } else {
      // Add the supported codecs as if they came from the component manifest.
      // This list must match the CDM that is being bundled with Chrome.
      audio_codecs_supported->push_back(media::AudioCodec::kOpus);
      audio_codecs_supported->push_back(media::AudioCodec::kVorbis);
#if BUILDFLAG(USE_PROPRIETARY_CODECS)
      audio_codecs_supported->push_back(media::AudioCodec::kAAC);
#endif  // BUILDFLAG(USE_PROPRIETARY_CODECS)
      codecs_supported->push_back(media::VideoCodec::kVP8);
      codecs_supported->push_back(media::VideoCodec::kVP9);
#if BUILDFLAG(USE_PROPRIETARY_CODECS)
      codecs_supported->push_back(media::VideoCodec::kH264);
#endif  // BUILDFLAG(USE_PROPRIETARY_CODECS)

      // TODO(crbug.com/767941): Push persistent-license support info here once
      // we check in a new CDM that supports it on Linux.
      session_types_supported->insert(media::CdmSessionType::kTemporary);
#if BUILDFLAG(IS_CHROMEOS)
      session_types_supported->insert(
          media::CdmSessionType::kPersistentLicense);
#endif  // BUILDFLAG(IS_CHROMEOS)

      modes_supported->insert(media::EncryptionScheme::kCenc);
    }

    return true;
  }

  return false;
}
#endif  // BUILDFLAG(ENABLE_WIDEVINE)

}  // namespace

ElectronContentClient::ElectronContentClient() = default;

ElectronContentClient::~ElectronContentClient() = default;

std::u16string ElectronContentClient::GetLocalizedString(int message_id) {
  return l10n_util::GetStringUTF16(message_id);
}

std::string_view ElectronContentClient::GetDataResource(
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
  // Browser Process registration happens in
  // `api::Protocol::RegisterSchemesAsPrivileged`
  //
  // Renderer Process registration happens in `RendererClientBase`
  //
  // We use this for registration to network utility process
  if (IsUtilityProcess()) {
    const auto& cmd = *base::CommandLine::ForCurrentProcess();
    auto append_cli_schemes = [&cmd](auto& appendme, const auto key) {
      base::Extend(appendme, base::SplitString(cmd.GetSwitchValueASCII(key),
                                               ",", base::TRIM_WHITESPACE,
                                               base::SPLIT_WANT_NONEMPTY));
    };

    using namespace switches;
    append_cli_schemes(schemes->cors_enabled_schemes, kCORSSchemes);
    append_cli_schemes(schemes->csp_bypassing_schemes, kBypassCSPSchemes);
    append_cli_schemes(schemes->secure_schemes, kSecureSchemes);
    append_cli_schemes(schemes->service_worker_schemes, kServiceWorkerSchemes);
    append_cli_schemes(schemes->standard_schemes, kStandardSchemes);
  }

  if (electron::fuses::IsGrantFileProtocolExtraPrivilegesEnabled()) {
    schemes->service_worker_schemes.emplace_back(url::kFileScheme);
  }

#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
  schemes->standard_schemes.push_back(extensions::kExtensionScheme);
  schemes->savable_schemes.push_back(extensions::kExtensionScheme);
  schemes->secure_schemes.push_back(extensions::kExtensionScheme);
  schemes->service_worker_schemes.push_back(extensions::kExtensionScheme);
  schemes->cors_enabled_schemes.push_back(extensions::kExtensionScheme);
  schemes->csp_bypassing_schemes.push_back(extensions::kExtensionScheme);
#endif
}

void ElectronContentClient::AddPlugins(
    std::vector<content::WebPluginInfo>* plugins) {
#if BUILDFLAG(ENABLE_PDF_VIEWER)
  static constexpr char16_t kPDFPluginName[] = u"Chromium PDF Plugin";
  static constexpr char16_t kPDFPluginDescription[] = u"Built-in PDF viewer";
  static constexpr char kPDFPluginExtension[] = "pdf";
  static constexpr char kPDFPluginExtensionDescription[] =
      "Portable Document Format";

  content::WebPluginInfo pdf_info;
  pdf_info.name = kPDFPluginName;
  // This isn't a real file path; it's just used as a unique identifier.
  static constexpr std::string_view kPdfPluginPath = "internal-pdf-viewer";
  pdf_info.path = base::FilePath::FromASCII(kPdfPluginPath);
  pdf_info.desc = kPDFPluginDescription;
  content::WebPluginMimeType pdf_mime_type(pdf::kInternalPluginMimeType,
                                           kPDFPluginExtension,
                                           kPDFPluginExtensionDescription);
  pdf_info.mime_types.push_back(pdf_mime_type);
  pdf_info.type = content::WebPluginInfo::PLUGIN_TYPE_BROWSER_INTERNAL_PLUGIN;
  plugins->push_back(pdf_info);
#endif  // BUILDFLAG(ENABLE_PDF_VIEWER)
}

void ElectronContentClient::AddContentDecryptionModules(
    std::vector<content::CdmInfo>* cdms,
    std::vector<media::CdmHostFilePath>* cdm_host_file_paths) {
  if (cdms) {
#if BUILDFLAG(ENABLE_WIDEVINE)
    base::FilePath cdm_path;
    std::vector<media::AudioCodec> audio_codecs_supported;
    std::vector<media::VideoCodec> video_codecs_supported;
    base::flat_set<media::CdmSessionType> session_types_supported;
    base::flat_set<media::EncryptionScheme> encryption_modes_supported;
    std::optional<base::Version> cdm_version;
    if (IsWidevineAvailable(&cdm_path, &audio_codecs_supported,
                            &video_codecs_supported, &session_types_supported,
                            &encryption_modes_supported, &cdm_version)) {
      base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
      // CdmInfo needs |path| to be the actual Widevine library,
      // not the adapter, so adjust as necessary. It will be in the
      // same directory as the installed adapter.
      base::Version version;
      if (cdm_version.has_value() && cdm_version->IsValid()) {
        version = *cdm_version;
      } else {
        const std::string version_string =
            command_line->GetSwitchValueASCII(switches::kWidevineCdmVersion);
        base::Version parsed(version_string);
        if (parsed.IsValid()) {
          version = parsed;
        }
      }

      media::CdmCapability::VideoCodecMap video_codec_map;
      for (media::VideoCodec codec : video_codecs_supported) {
        video_codec_map.emplace(codec, media::VideoCodecInfo());
      }

      media::CdmCapability capability(
          audio_codecs_supported, std::move(video_codec_map),
          encryption_modes_supported, session_types_supported, version);

      cdms->push_back(content::CdmInfo(
          kWidevineKeySystem, content::CdmInfo::Robustness::kSoftwareSecure,
          capability,
          /*supports_sub_key_systems=*/true, kWidevineCdmDisplayName,
          kWidevineCdmType, cdm_path));
    }
#endif  // BUILDFLAG(ENABLE_WIDEVINE)
  }
}

}  // namespace electron
