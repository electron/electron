// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/renderer/media/chrome_key_systems.h"

#include <stddef.h>

#include <string>
#include <vector>

#include "base/logging.h"
#include "base/strings/string16.h"
#include "base/strings/string_split.h"
#include "base/strings/utf_string_conversions.h"
#include "components/cdm/renderer/widevine_key_system_properties.h"
#include "content/public/renderer/render_thread.h"
#include "media/base/eme_constants.h"
#include "media/base/key_system_properties.h"
#include "media/media_buildflags.h"

#if BUILDFLAG(ENABLE_LIBRARY_CDMS)
#include "content/public/renderer/key_system_support.h"
#include "media/base/video_codecs.h"
#endif

#include "widevine_cdm_version.h"  // In SHARED_INTERMEDIATE_DIR.

// The following must be after widevine_cdm_version.h.

#if defined(WIDEVINE_CDM_AVAILABLE) && defined(WIDEVINE_CDM_MIN_GLIBC_VERSION)
#include <gnu/libc-version.h>
#include "base/version.h"
#endif

using media::KeySystemProperties;
using media::SupportedCodecs;

#if BUILDFLAG(ENABLE_LIBRARY_CDMS)
static const char kExternalClearKeyPepperType[] =
    "application/x-ppapi-clearkey-cdm";

// KeySystemProperties implementation for external Clear Key systems.
class ExternalClearKeyProperties : public KeySystemProperties {
 public:
  explicit ExternalClearKeyProperties(const std::string& key_system_name)
      : key_system_name_(key_system_name) {}

  std::string GetKeySystemName() const override { return key_system_name_; }
  bool IsSupportedInitDataType(
      media::EmeInitDataType init_data_type) const override {
    switch (init_data_type) {
      case media::EmeInitDataType::WEBM:
      case media::EmeInitDataType::KEYIDS:
        return true;

      case media::EmeInitDataType::CENC:
#if BUILDFLAG(USE_PROPRIETARY_CODECS)
        return true;
#else
        return false;
#endif  // BUILDFLAG(USE_PROPRIETARY_CODECS)

      case media::EmeInitDataType::UNKNOWN:
        return false;
    }
    NOTREACHED();
    return false;
  }

  SupportedCodecs GetSupportedCodecs() const override {
#if BUILDFLAG(USE_PROPRIETARY_CODECS)
    return media::EME_CODEC_MP4_ALL | media::EME_CODEC_WEBM_ALL;
#else
    return media::EME_CODEC_WEBM_ALL;
#endif
  }

  media::EmeConfigRule GetRobustnessConfigRule(
      media::EmeMediaType media_type,
      const std::string& requested_robustness) const override {
    return requested_robustness.empty() ? media::EmeConfigRule::SUPPORTED
                                        : media::EmeConfigRule::NOT_SUPPORTED;
  }

  // Persistent license sessions are faked.
  media::EmeSessionTypeSupport GetPersistentLicenseSessionSupport()
      const override {
    return media::EmeSessionTypeSupport::SUPPORTED;
  }

  media::EmeSessionTypeSupport GetPersistentReleaseMessageSessionSupport()
      const override {
    return media::EmeSessionTypeSupport::NOT_SUPPORTED;
  }

  media::EmeFeatureSupport GetPersistentStateSupport() const override {
    return media::EmeFeatureSupport::REQUESTABLE;
  }

  media::EmeFeatureSupport GetDistinctiveIdentifierSupport() const override {
    return media::EmeFeatureSupport::NOT_SUPPORTED;
  }

  std::string GetPepperType() const override {
    return kExternalClearKeyPepperType;
  }

 private:
  const std::string key_system_name_;
};

// External Clear Key (used for testing).
static void AddExternalClearKey(
    std::vector<std::unique_ptr<KeySystemProperties>>* concrete_key_systems) {
  static const char kExternalClearKeyKeySystem[] =
      "org.chromium.externalclearkey";
  static const char kExternalClearKeyDecryptOnlyKeySystem[] =
      "org.chromium.externalclearkey.decryptonly";
  static const char kExternalClearKeyFileIOTestKeySystem[] =
      "org.chromium.externalclearkey.fileiotest";
  static const char kExternalClearKeyInitializeFailKeySystem[] =
      "org.chromium.externalclearkey.initializefail";
  static const char kExternalClearKeyCrashKeySystem[] =
      "org.chromium.externalclearkey.crash";

  std::vector<media::VideoCodec> supported_video_codecs;
  bool supports_persistent_license;
  if (!content::IsKeySystemSupported(kExternalClearKeyKeySystem,
                                     &supported_video_codecs,
                                     &supports_persistent_license)) {
    return;
  }

  concrete_key_systems->emplace_back(
      new ExternalClearKeyProperties(kExternalClearKeyKeySystem));

  // Add support of decrypt-only mode in ClearKeyCdm.
  concrete_key_systems->emplace_back(
      new ExternalClearKeyProperties(kExternalClearKeyDecryptOnlyKeySystem));

  // A key system that triggers FileIO test in ClearKeyCdm.
  concrete_key_systems->emplace_back(
      new ExternalClearKeyProperties(kExternalClearKeyFileIOTestKeySystem));

  // A key system that Chrome thinks is supported by ClearKeyCdm, but actually
  // will be refused by ClearKeyCdm. This is to test the CDM initialization
  // failure case.
  concrete_key_systems->emplace_back(
      new ExternalClearKeyProperties(kExternalClearKeyInitializeFailKeySystem));

  // A key system that triggers a crash in ClearKeyCdm.
  concrete_key_systems->emplace_back(
      new ExternalClearKeyProperties(kExternalClearKeyCrashKeySystem));
}

#if defined(WIDEVINE_CDM_AVAILABLE)
static void AddPepperBasedWidevine(
    std::vector<std::unique_ptr<KeySystemProperties>>* concrete_key_systems) {
#if defined(WIDEVINE_CDM_MIN_GLIBC_VERSION)
  Version glibc_version(gnu_get_libc_version());
  DCHECK(glibc_version.IsValid());
  if (glibc_version < base::Version(WIDEVINE_CDM_MIN_GLIBC_VERSION))
    return;
#endif  // defined(WIDEVINE_CDM_MIN_GLIBC_VERSION)

  std::vector<media::VideoCodec> supported_video_codecs;
  bool supports_persistent_license = false;
  if (!content::IsKeySystemSupported(kWidevineKeySystem,
                                     &supported_video_codecs,
                                     &supports_persistent_license)) {
    DVLOG(1) << "Widevine CDM is not currently available.";
    return;
  }

  SupportedCodecs supported_codecs = media::EME_CODEC_NONE;

  // Audio codecs are always supported.
  // TODO(sandersd): Distinguish these from those that are directly supported,
  // as those may offer a higher level of protection.
  supported_codecs |= media::EME_CODEC_WEBM_OPUS;
  supported_codecs |= media::EME_CODEC_WEBM_VORBIS;
#if BUILDFLAG(USE_PROPRIETARY_CODECS)
  supported_codecs |= media::EME_CODEC_MP4_AAC;
#endif  // BUILDFLAG(USE_PROPRIETARY_CODECS)

  // Video codecs are determined by what was registered for the CDM.
  for (const auto& codec : supported_video_codecs) {
    switch (codec) {
      case media::VideoCodec::kCodecVP8:
        supported_codecs |= media::EME_CODEC_WEBM_VP8;
        break;
      case media::VideoCodec::kCodecVP9:
        supported_codecs |= media::EME_CODEC_WEBM_VP9;
        supported_codecs |= media::EME_CODEC_COMMON_VP9;
        break;
#if BUILDFLAG(USE_PROPRIETARY_CODECS)
      case media::VideoCodec::kCodecH264:
        supported_codecs |= media::EME_CODEC_MP4_AVC1;
        break;
#endif  // BUILDFLAG(USE_PROPRIETARY_CODECS)
      default:
        DVLOG(1) << "Unexpected supported codec: " << GetCodecName(codec);
        break;
    }
  }

  using Robustness = cdm::WidevineKeySystemProperties::Robustness;
  concrete_key_systems->emplace_back(new cdm::WidevineKeySystemProperties(
      supported_codecs,
#if defined(OS_CHROMEOS)
      media::EmeRobustness::HW_SECURE_ALL,  // Maximum audio robustness.
      media::EmeRobustness::HW_SECURE_ALL,  // Maximim video robustness.
      media::EmeSessionTypeSupport::
          SUPPORTED_WITH_IDENTIFIER,  // Persistent-license.
      media::EmeSessionTypeSupport::
          NOT_SUPPORTED,                        // Persistent-release-message.
      media::EmeFeatureSupport::REQUESTABLE,    // Persistent state.
      media::EmeFeatureSupport::REQUESTABLE));  // Distinctive identifier.
#else                                           // (Desktop)
      Robustness::SW_SECURE_CRYPTO,                 // Maximum audio robustness.
      Robustness::SW_SECURE_DECODE,                 // Maximum video robustness.
      media::EmeSessionTypeSupport::NOT_SUPPORTED,  // persistent-license.
      media::EmeSessionTypeSupport::
          NOT_SUPPORTED,                          // persistent-release-message.
      media::EmeFeatureSupport::REQUESTABLE,      // Persistent state.
      media::EmeFeatureSupport::NOT_SUPPORTED));  // Distinctive identifier.
#endif                                          // defined(OS_CHROMEOS)
}
#endif  // defined(WIDEVINE_CDM_AVAILABLE)
#endif  // BUILDFLAG(ENABLE_LIBRARY_CDMS)

void AddChromeKeySystems(
    std::vector<std::unique_ptr<KeySystemProperties>>* key_systems_properties) {
#if BUILDFLAG(ENABLE_LIBRARY_CDMS)
  AddExternalClearKey(key_systems_properties);

#if defined(WIDEVINE_CDM_AVAILABLE)
  AddPepperBasedWidevine(key_systems_properties);
#endif  // defined(WIDEVINE_CDM_AVAILABLE)
#endif  // BUILDFLAG(ENABLE_LIBRARY_CDMS)
}
