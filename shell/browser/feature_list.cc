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
#include "printing/buildflags/buildflags.h"
#include "sandbox/policy/features.h"
#include "services/network/public/cpp/features.h"
#include "third_party/blink/public/common/features.h"
#include "ui/accessibility/ax_features.mojom-features.h"

#if BUILDFLAG(IS_MAC)
#include "device/base/features.h"  // nogncheck
#endif

#if BUILDFLAG(ENABLE_PDF_VIEWER)
#include "pdf/pdf_features.h"
#endif

#if BUILDFLAG(IS_LINUX)
#include "printing/printing_features.h"
#endif

#if BUILDFLAG(IS_WIN)
#include "ui/views/views_features.h"
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
      std::string(",") + features::kSpareRendererForSitePerProcess.name +
      // See https://chromium-review.googlesource.com/c/chromium/src/+/6487926
      // this breaks PDFs locally as we don't have GLIC infra enabled.
      std::string(",") + ax::mojom::features::kScreenAIOCREnabled.name +
      // See https://chromium-review.googlesource.com/c/chromium/src/+/6626905
      // Needed so that ElectronBrowserClient::RegisterPendingSiteInstance does
      // not throw a check.
      std::string(", TraceSiteInstanceGetProcessCreation") +
      // See https://chromium-review.googlesource.com/c/chromium/src/+/6910012
      // Needed until we rework some of our logic and checks to enable this
      // properly.
      std::string(",") + network::features::kLocalNetworkAccessChecks.name +
      // See 4803165: Enable suppressing input event dispatch while
      // paint-holding. Needed to prevent spurious input event handling
      // failures.
      // TODO(codebytere): Figure out how to properly wait for paint-hold.
      std::string(",") +
      blink::features::kDropInputEventsWhilePaintHolding.name;

#if BUILDFLAG(IS_WIN)
  // Refs https://issues.chromium.org/issues/401996981
  // TODO(deepak1556): Remove this once test added in
  // https://github.com/electron/electron/pull/12904
  // can work without this feature.
  enable_features += std::string(",") +
                     views::features::kEnableTransparentHwndEnlargement.name;

  // See https://chromium-review.googlesource.com/c/chromium/src/+/7204292
  // This feature causes the following sandbox failure on Windows:
  // sandbox\policy\win\sandbox_win.cc:777 Sandbox cannot access executable
  // electron.exe. Check filesystem permissions are valid.
  // See https://bit.ly/31yqMJR.: Access is denied. (0x5)
  disable_features +=
      std::string(",") + sandbox::policy::features::kNetworkServiceSandbox.name;
#endif

#if BUILDFLAG(IS_MAC)
  disable_features +=
      // MacWebContentsOcclusion is causing some odd visibility
      // issues with multiple web contents
      std::string(",") + features::kMacWebContentsOcclusion.name;
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
