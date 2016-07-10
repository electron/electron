// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/renderer/media/chrome_key_systems.h"

#include <string>
#include <vector>

#include "base/logging.h"
#include "base/strings/string16.h"
#include "base/strings/string_split.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/common/widevine_cdm_messages.h"
#include "components/cdm/renderer/widevine_key_systems.h"
#include "content/public/renderer/render_thread.h"
#include "media/base/eme_constants.h"

#if defined(OS_ANDROID)
#include "components/cdm/renderer/android_key_systems.h"
#endif

// #include "widevine_cdm_version.h" // In SHARED_INTERMEDIATE_DIR.
#include "third_party/widevine/cdm/stub/widevine_cdm_version.h"

// The following must be after widevine_cdm_version.h.

#if defined(WIDEVINE_CDM_AVAILABLE) && defined(WIDEVINE_CDM_MIN_GLIBC_VERSION)
#include <gnu/libc-version.h>
#include "base/version.h"
#endif

using media::KeySystemInfo;
using media::SupportedCodecs;

#if defined(ENABLE_PEPPER_CDMS)
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
    std::vector<KeySystemInfo>* concrete_key_systems) {
#if defined(WIDEVINE_CDM_MIN_GLIBC_VERSION)
  Version glibc_version(gnu_get_libc_version());
  DCHECK(glibc_version.IsValid());
  if (glibc_version.IsOlderThan(WIDEVINE_CDM_MIN_GLIBC_VERSION))
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

  for (auto& codec : codecs) {
    if (codec == kCdmSupportedCodecVp8)
      supported_codecs |= media::EME_CODEC_WEBM_VP8;
    if (codec == kCdmSupportedCodecVp9)
      supported_codecs |= media::EME_CODEC_WEBM_VP9;
#if defined(USE_PROPRIETARY_CODECS)
    if (codec == kCdmSupportedCodecAvc1)
      supported_codecs |= media::EME_CODEC_MP4_AVC1;
#endif  // defined(USE_PROPRIETARY_CODECS)
  }

  cdm::AddWidevineWithCodecs(
      supported_codecs,
#if defined(OS_CHROMEOS)
      media::EmeRobustness::HW_SECURE_ALL,  // Maximum audio robustness.
      media::EmeRobustness::HW_SECURE_ALL,  // Maximim video robustness.
      media::EmeSessionTypeSupport::
          SUPPORTED_WITH_IDENTIFIER,  // Persistent-license.
      media::EmeSessionTypeSupport::
          NOT_SUPPORTED,                      // Persistent-release-message.
      media::EmeFeatureSupport::REQUESTABLE,  // Persistent state.
      media::EmeFeatureSupport::REQUESTABLE,  // Distinctive identifier.
#else   // (Desktop)
      media::EmeRobustness::SW_SECURE_CRYPTO,       // Maximum audio robustness.
      media::EmeRobustness::SW_SECURE_DECODE,       // Maximum video robustness.
      media::EmeSessionTypeSupport::NOT_SUPPORTED,  // persistent-license.
      media::EmeSessionTypeSupport::
          NOT_SUPPORTED,                        // persistent-release-message.
      media::EmeFeatureSupport::REQUESTABLE,    // Persistent state.
      media::EmeFeatureSupport::NOT_SUPPORTED,  // Distinctive identifier.
#endif  // defined(OS_CHROMEOS)
      concrete_key_systems);
}
#endif  // defined(WIDEVINE_CDM_AVAILABLE)
#endif  // defined(ENABLE_PEPPER_CDMS)

void AddChromeKeySystems(std::vector<KeySystemInfo>* key_systems_info) {
#if defined(ENABLE_PEPPER_CDMS)
#if defined(WIDEVINE_CDM_AVAILABLE)
  AddPepperBasedWidevine(key_systems_info);
#endif  // defined(WIDEVINE_CDM_AVAILABLE)
#endif  // defined(ENABLE_PEPPER_CDMS)
}
