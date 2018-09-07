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
#include "build/build_config.h"
#if 0
#include "chrome/renderer/chrome_render_thread_observer.h"
#endif
#include "components/cdm/renderer/external_clear_key_key_system_properties.h"
#include "components/cdm/renderer/widevine_key_system_properties.h"
#include "content/public/renderer/render_thread.h"
#include "media/base/eme_constants.h"
#include "media/base/key_system_properties.h"
#include "media/media_features.h"
#include "third_party/widevine/cdm/widevine_cdm_common.h"

#if BUILDFLAG(ENABLE_LIBRARY_CDMS)
#include "base/feature_list.h"
#include "content/public/renderer/key_system_support.h"
#include "media/base/media_switches.h"
#include "media/base/video_codecs.h"
#endif

#if defined(WIDEVINE_CDM_AVAILABLE) && defined(WIDEVINE_CDM_MIN_GLIBC_VERSION)
#include <gnu/libc-version.h>
#include "base/version.h"
#endif

using media::EmeFeatureSupport;
using media::EmeSessionTypeSupport;
using media::KeySystemProperties;
using media::SupportedCodecs;

#if BUILDFLAG(ENABLE_LIBRARY_CDMS)

// External Clear Key (used for testing).
static void AddExternalClearKey(
    std::vector<std::unique_ptr<KeySystemProperties>>* concrete_key_systems) {
  // TODO(xhwang): Move these into an array so we can use a for loop to add
  // supported key systems below.
  static const char kExternalClearKeyKeySystem[] =
      "org.chromium.externalclearkey";
  static const char kExternalClearKeyDecryptOnlyKeySystem[] =
      "org.chromium.externalclearkey.decryptonly";
  static const char kExternalClearKeyMessageTypeTestKeySystem[] =
      "org.chromium.externalclearkey.messagetypetest";
  static const char kExternalClearKeyFileIOTestKeySystem[] =
      "org.chromium.externalclearkey.fileiotest";
  static const char kExternalClearKeyOutputProtectionTestKeySystem[] =
      "org.chromium.externalclearkey.outputprotectiontest";
  static const char kExternalClearKeyPlatformVerificationTestKeySystem[] =
      "org.chromium.externalclearkey.platformverificationtest";
  static const char kExternalClearKeyInitializeFailKeySystem[] =
      "org.chromium.externalclearkey.initializefail";
  static const char kExternalClearKeyCrashKeySystem[] =
      "org.chromium.externalclearkey.crash";
  static const char kExternalClearKeyVerifyCdmHostTestKeySystem[] =
      "org.chromium.externalclearkey.verifycdmhosttest";
  static const char kExternalClearKeyStorageIdTestKeySystem[] =
      "org.chromium.externalclearkey.storageidtest";
  static const char kExternalClearKeyDifferentGuidTestKeySystem[] =
      "org.chromium.externalclearkey.differentguid";
  static const char kExternalClearKeyCdmProxyTestKeySystem[] =
      "org.chromium.externalclearkey.cdmproxytest";

  std::vector<media::VideoCodec> supported_video_codecs;
  bool supports_persistent_license;
  if (!content::IsKeySystemSupported(kExternalClearKeyKeySystem,
                                     &supported_video_codecs,
                                     &supports_persistent_license)) {
    return;
  }

  concrete_key_systems->emplace_back(
      new cdm::ExternalClearKeyProperties(kExternalClearKeyKeySystem));

  // Add support of decrypt-only mode in ClearKeyCdm.
  concrete_key_systems->emplace_back(new cdm::ExternalClearKeyProperties(
      kExternalClearKeyDecryptOnlyKeySystem));

  // A key system that triggers various types of messages in ClearKeyCdm.
  concrete_key_systems->emplace_back(new cdm::ExternalClearKeyProperties(
      kExternalClearKeyMessageTypeTestKeySystem));

  // A key system that triggers the FileIO test in ClearKeyCdm.
  concrete_key_systems->emplace_back(new cdm::ExternalClearKeyProperties(
      kExternalClearKeyFileIOTestKeySystem));

  // A key system that triggers the output protection test in ClearKeyCdm.
  concrete_key_systems->emplace_back(new cdm::ExternalClearKeyProperties(
      kExternalClearKeyOutputProtectionTestKeySystem));

  // A key system that triggers the platform verification test in ClearKeyCdm.
  concrete_key_systems->emplace_back(new cdm::ExternalClearKeyProperties(
      kExternalClearKeyPlatformVerificationTestKeySystem));

  // A key system that Chrome thinks is supported by ClearKeyCdm, but actually
  // will be refused by ClearKeyCdm. This is to test the CDM initialization
  // failure case.
  concrete_key_systems->emplace_back(new cdm::ExternalClearKeyProperties(
      kExternalClearKeyInitializeFailKeySystem));

  // A key system that triggers a crash in ClearKeyCdm.
  concrete_key_systems->emplace_back(
      new cdm::ExternalClearKeyProperties(kExternalClearKeyCrashKeySystem));

  // A key system that triggers the verify host files test in ClearKeyCdm.
  concrete_key_systems->emplace_back(new cdm::ExternalClearKeyProperties(
      kExternalClearKeyVerifyCdmHostTestKeySystem));

  // A key system that fetches the Storage ID in ClearKeyCdm.
  concrete_key_systems->emplace_back(new cdm::ExternalClearKeyProperties(
      kExternalClearKeyStorageIdTestKeySystem));

  // A key system that is registered with a different CDM GUID.
  concrete_key_systems->emplace_back(new cdm::ExternalClearKeyProperties(
      kExternalClearKeyDifferentGuidTestKeySystem));

  // A key system that triggers CDM Proxy test in ClearKeyCdm.
  concrete_key_systems->emplace_back(new cdm::ExternalClearKeyProperties(
      kExternalClearKeyCdmProxyTestKeySystem));
}

#if defined(WIDEVINE_CDM_AVAILABLE)
// Returns persistent-license session support.
EmeSessionTypeSupport GetPersistentLicenseSupport(bool supported_by_the_cdm) {
#if 0
  // Do not support persistent-license if the process cannot persist data.
  // TODO(crbug.com/457487): Have a better plan on this. See bug for details.
  if (ChromeRenderThreadObserver::is_incognito_process()) {
    DVLOG(2) << __func__ << ": Not supported in incognito process.";
    return EmeSessionTypeSupport::NOT_SUPPORTED;
  }
#endif

  if (!supported_by_the_cdm) {
    DVLOG(2) << __func__ << ": Not supported by the CDM.";
    return EmeSessionTypeSupport::NOT_SUPPORTED;
  }

// On ChromeOS, platform verification is similar to CDM host verification.
#if BUILDFLAG(ENABLE_CDM_HOST_VERIFICATION) || defined(OS_CHROMEOS)
  bool cdm_host_verification_potentially_supported = true;
#else
  bool cdm_host_verification_potentially_supported = false;
#endif

  // If we are sure CDM host verification is NOT supported, we should not
  // support persistent-license.
  if (!cdm_host_verification_potentially_supported) {
    DVLOG(2) << __func__ << ": Not supported without CDM host verification.";
    return EmeSessionTypeSupport::NOT_SUPPORTED;
  }

#if defined(OS_CHROMEOS)
  // On ChromeOS, platform verification (similar to CDM host verification)
  // requires identifier to be allowed.
  // TODO(jrummell): Currently the ChromeOS CDM does not require storage ID
  // to support persistent license. Update this logic when the new CDM requires
  // storage ID.
  return EmeSessionTypeSupport::SUPPORTED_WITH_IDENTIFIER;
#elif BUILDFLAG(ENABLE_CDM_STORAGE_ID)
  // On other platforms, we require storage ID to support persistent license.
  return EmeSessionTypeSupport::SUPPORTED;
#else
  // Storage ID not implemented, so no support for persistent license.
  DVLOG(2) << __func__ << ": Not supported without CDM storage ID.";
  return EmeSessionTypeSupport::NOT_SUPPORTED;
#endif  // defined(OS_CHROMEOS)
}

static void AddPepperBasedWidevine(
    std::vector<std::unique_ptr<KeySystemProperties>>* concrete_key_systems) {
#if defined(WIDEVINE_CDM_MIN_GLIBC_VERSION)
  base::Version glibc_version(gnu_get_libc_version());
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

  EmeSessionTypeSupport persistent_license_support =
      GetPersistentLicenseSupport(supports_persistent_license);

  using Robustness = cdm::WidevineKeySystemProperties::Robustness;

  concrete_key_systems->emplace_back(new cdm::WidevineKeySystemProperties(
      supported_codecs,
#if defined(OS_CHROMEOS)
      Robustness::HW_SECURE_ALL,             // Maximum audio robustness.
      Robustness::HW_SECURE_ALL,             // Maximum video robustness.
      persistent_license_support,            // Persistent-license.
      EmeSessionTypeSupport::NOT_SUPPORTED,  // Persistent-release-message.
      EmeFeatureSupport::REQUESTABLE,        // Persistent state.
      EmeFeatureSupport::REQUESTABLE));      // Distinctive identifier.
#else                                        // Desktop
      Robustness::SW_SECURE_CRYPTO,          // Maximum audio robustness.
      Robustness::SW_SECURE_DECODE,          // Maximum video robustness.
      persistent_license_support,            // persistent-license.
      EmeSessionTypeSupport::NOT_SUPPORTED,  // persistent-release-message.
      EmeFeatureSupport::REQUESTABLE,        // Persistent state.
      EmeFeatureSupport::NOT_SUPPORTED));    // Distinctive identifier.
#endif                                       // defined(OS_CHROMEOS)
}
#endif  // defined(WIDEVINE_CDM_AVAILABLE)
#endif  // BUILDFLAG(ENABLE_LIBRARY_CDMS)

void AddChromeKeySystems(
    std::vector<std::unique_ptr<KeySystemProperties>>* key_systems_properties) {
#if BUILDFLAG(ENABLE_LIBRARY_CDMS)
  if (base::FeatureList::IsEnabled(media::kExternalClearKeyForTesting))
    AddExternalClearKey(key_systems_properties);

#if defined(WIDEVINE_CDM_AVAILABLE)
  AddPepperBasedWidevine(key_systems_properties);
#endif  // defined(WIDEVINE_CDM_AVAILABLE)

#endif  // BUILDFLAG(ENABLE_LIBRARY_CDMS)
}
