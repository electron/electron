// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "electron/shell/browser/feature_list.h"

#include <string>

#include "base/base_switches.h"
#include "base/command_line.h"
#include "base/feature_list.h"
#include "base/metrics/field_trial.h"
#include "components/spellcheck/common/spellcheck_features.h"
#include "content/common/features.h"
#include "content/public/common/content_features.h"
#include "electron/buildflags/buildflags.h"
#include "media/base/media_switches.h"
#include "net/base/features.h"
#include "services/network/public/cpp/features.h"
#include "third_party/blink/public/common/features.h"

#if BUILDFLAG(IS_MAC)
#include "device/base/features.h"  // nogncheck
#endif

#if BUILDFLAG(ENABLE_PDF_VIEWER)
#include "pdf/pdf_features.h"
#endif

#if BUILDFLAG(IS_LINUX)
#include "printing/printing_features.h"
#endif

namespace electron {

void InitializeFeatureList() {
  auto* cmd_line = base::CommandLine::ForCurrentProcess();
  auto enable_features =
      cmd_line->GetSwitchValueASCII(::switches::kEnableFeatures);
  auto disable_features =
      cmd_line->GetSwitchValueASCII(::switches::kDisableFeatures);
  // Disable creation of spare renderer process with site-per-process mode,
  // it interferes with our process preference tracking for non sandboxed mode.
  // Can be reenabled when our site instance policy is aligned with chromium
  // when node integration is enabled.
  disable_features +=
      std::string(",") + features::kSpareRendererForSitePerProcess.name;

#if BUILDFLAG(IS_WIN)
  disable_features +=
      // Delayed spellcheck initialization is causing the
      // 'custom dictionary word list API' spec to crash.
      std::string(",") + spellcheck::kWinDelaySpellcheckServiceInit.name;
#endif

#if BUILDFLAG(IS_MAC)
  disable_features +=
      // MacWebContentsOcclusion is causing some odd visibility
      // issues with multiple web contents
      std::string(",") + features::kMacWebContentsOcclusion.name;
#endif

#if BUILDFLAG(IS_LINUX)
  disable_features +=
      // EnableOopPrintDrivers is still a bit half-baked on Linux and
      // causes crashes when trying to show dialogs.
      // TODO(codebytere): figure out how to re-enable this with our patches.
      std::string(",") + printing::features::kEnableOopPrintDrivers.name;
#endif

#if BUILDFLAG(ENABLE_PDF_VIEWER)
  // Enable window.showSaveFilePicker api for saving pdf files.
  // Refs https://issues.chromium.org/issues/373852607
  enable_features +=
      std::string(",") + chrome_pdf::features::kPdfUseShowSaveFilePicker.name;
#endif

  std::string platform_specific_enable_features =
      EnablePlatformSpecificFeatures();
  if (platform_specific_enable_features.size() > 0) {
    enable_features += std::string(",") + platform_specific_enable_features;
  }
  std::string platform_specific_disable_features =
      DisablePlatformSpecificFeatures();
  if (platform_specific_disable_features.size() > 0) {
    disable_features += std::string(",") + platform_specific_disable_features;
  }
  base::FeatureList::InitInstance(enable_features, disable_features);
}

void InitializeFieldTrials() {
  auto* cmd_line = base::CommandLine::ForCurrentProcess();
  auto force_fieldtrials =
      cmd_line->GetSwitchValueASCII(::switches::kForceFieldTrials);

  base::FieldTrialList::CreateTrialsFromString(force_fieldtrials);
}

#if !BUILDFLAG(IS_MAC)
std::string EnablePlatformSpecificFeatures() {
  return "";
}
std::string DisablePlatformSpecificFeatures() {
  return "";
}
#endif

}  // namespace electron
