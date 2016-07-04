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
#include "chrome/common/widevine_cdm_messages.h"
#include "components/cdm/renderer/widevine_key_system_properties.h"
#include "content/public/renderer/render_thread.h"
#include "media/base/eme_constants.h"
#include "media/base/key_system_properties.h"

// #include "widevine_cdm_version.h" // In SHARED_INTERMEDIATE_DIR.
#include "third_party/widevine/cdm/stub/widevine_cdm_version.h"

// The following must be after widevine_cdm_version.h.

#if defined(WIDEVINE_CDM_AVAILABLE) && defined(WIDEVINE_CDM_MIN_GLIBC_VERSION)
#include <gnu/libc-version.h>
#include "base/version.h"
#endif

using media::KeySystemProperties;
using media::SupportedCodecs;

#if defined(ENABLE_PEPPER_CDMS)
static const char kExternalClearKeyPepperType[] =
    "application/x-ppapi-clearkey-cdm";

static bool IsPepperCdmAvailable(
    const std::string& pepper_type,
    std::vector<base::string16>* additional_param_names,
    std::vector<base::string16>* additional_param_values) {
  bool is_available = false;
  content::RenderThread::Get()->Send(
      new ChromeViewHostMsg_IsInternalPluginAvailableForMimeType(
          pepper_type,
          &is_available,
          additional_param_names,
          additional_param_values));

  return is_available;
}

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
#if defined(USE_PROPRIETARY_CODECS)
        return true;
#else
        return false;
#endif  // defined(USE_PROPRIETARY_CODECS)

      case media::EmeInitDataType::UNKNOWN:
        return false;
    }
    NOTREACHED();
    return false;
  }

  SupportedCodecs GetSupportedCodecs() const override {
#if defined(USE_PROPRIETARY_CODECS)
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

  std::vector<base::string16> additional_param_names;
  std::vector<base::string16> additional_param_values;
  if (!IsPepperCdmAvailable(kExternalClearKeyPepperType,
                            &additional_param_names,
                            &additional_param_values)) {
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
// This function finds "codecs" and parses the value into the vector |codecs|.
// Converts the codec strings to UTF-8 since we only expect ASCII strings and
// this simplifies the rest of the code in this file.
void GetSupportedCodecsForPepperCdm(
    const std::vector<base::string16>& additional_param_names,
    const std::vector<base::string16>& additional_param_values,
    std::vector<std::string>* codecs) {
  DCHECK(codecs->empty());
  DCHECK_EQ(additional_param_names.size(), additional_param_values.size());
  for (size_t i = 0; i < additional_param_names.size(); ++i) {
    if (additional_param_names[i] ==
        base::ASCIIToUTF16(kCdmSupportedCodecsParamName)) {
      const base::string16& codecs_string16 = additional_param_values[i];
      std::string codecs_string;
      if (!base::UTF16ToUTF8(codecs_string16.c_str(),
                             codecs_string16.length(),
                             &codecs_string)) {
        DLOG(WARNING) << "Non-UTF-8 codecs string.";
        // Continue using the best effort conversion.
      }
      *codecs = base::SplitString(
          codecs_string, std::string(1, kCdmSupportedCodecsValueDelimiter),
          base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
      break;
    }
  }
}

static void AddPepperBasedWidevine(
    std::vector<std::unique_ptr<KeySystemProperties>>* concrete_key_systems) {
#if defined(WIDEVINE_CDM_MIN_GLIBC_VERSION)
  Version glibc_version(gnu_get_libc_version());
  DCHECK(glibc_version.IsValid());
  if (glibc_version < base::Version(WIDEVINE_CDM_MIN_GLIBC_VERSION))
    return;
#endif  // defined(WIDEVINE_CDM_MIN_GLIBC_VERSION)

  std::vector<base::string16> additional_param_names;
  std::vector<base::string16> additional_param_values;
  if (!IsPepperCdmAvailable(kWidevineCdmPluginMimeType,
                            &additional_param_names,
                            &additional_param_values)) {
    DVLOG(1) << "Widevine CDM is not currently available.";
    return;
  }

  std::vector<std::string> codecs;
  GetSupportedCodecsForPepperCdm(additional_param_names,
                                 additional_param_values,
                                 &codecs);

  SupportedCodecs supported_codecs = media::EME_CODEC_NONE;

  // Audio codecs are always supported.
  // TODO(sandersd): Distinguish these from those that are directly supported,
  // as those may offer a higher level of protection.
  supported_codecs |= media::EME_CODEC_WEBM_OPUS;
  supported_codecs |= media::EME_CODEC_WEBM_VORBIS;
#if defined(USE_PROPRIETARY_CODECS)
  supported_codecs |= media::EME_CODEC_MP4_AAC;
#endif  // defined(USE_PROPRIETARY_CODECS)

  for (size_t i = 0; i < codecs.size(); ++i) {
    if (codecs[i] == kCdmSupportedCodecVp8)
      supported_codecs |= media::EME_CODEC_WEBM_VP8;
    if (codecs[i] == kCdmSupportedCodecVp9)
      supported_codecs |= media::EME_CODEC_WEBM_VP9;
#if defined(USE_PROPRIETARY_CODECS)
    if (codecs[i] == kCdmSupportedCodecAvc1)
      supported_codecs |= media::EME_CODEC_MP4_AVC1;
    if (codecs[i] == kCdmSupportedCodecVp9)
      supported_codecs |= media::EME_CODEC_MP4_VP9;
#endif  // defined(USE_PROPRIETARY_CODECS)
  }

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
#else   // (Desktop)
      media::EmeRobustness::SW_SECURE_CRYPTO,       // Maximum audio robustness.
      media::EmeRobustness::SW_SECURE_DECODE,       // Maximum video robustness.
      media::EmeSessionTypeSupport::NOT_SUPPORTED,  // persistent-license.
      media::EmeSessionTypeSupport::
          NOT_SUPPORTED,                          // persistent-release-message.
      media::EmeFeatureSupport::REQUESTABLE,      // Persistent state.
      media::EmeFeatureSupport::NOT_SUPPORTED));  // Distinctive identifier.
#endif  // defined(OS_CHROMEOS)
}
#endif  // defined(WIDEVINE_CDM_AVAILABLE)
#endif  // defined(ENABLE_PEPPER_CDMS)

void AddChromeKeySystems(
    std::vector<std::unique_ptr<KeySystemProperties>>* key_systems_properties) {
#if defined(ENABLE_PEPPER_CDMS)
  AddExternalClearKey(key_systems_properties);

#if defined(WIDEVINE_CDM_AVAILABLE)
  AddPepperBasedWidevine(key_systems_properties);
#endif  // defined(WIDEVINE_CDM_AVAILABLE)
#endif  // defined(ENABLE_PEPPER_CDMS)
}
