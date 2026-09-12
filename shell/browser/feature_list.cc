// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "electron/shell/browser/feature_list.h"

#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

#include "base/base_switches.h"
#include "base/command_line.h"
#include "base/feature_list.h"
#include "base/metrics/field_trial.h"
#include "base/strings/string_util.h"
#include "components/spellcheck/common/spellcheck_features.h"
#include "components/unexportable_keys/features.h"
#include "content/public/common/content_features.h"
#include "electron/buildflags/buildflags.h"
#include "electron/fuses.h"
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
#include "ui/base/ui_base_features.h"
#endif

namespace electron {

void InitializeFeatureList() {
  auto* cmd_line = base::CommandLine::ForCurrentProcess();
  auto enable_features =
      cmd_line->GetSwitchValueASCII(::switches::kEnableFeatures);
  auto disable_features =
      cmd_line->GetSwitchValueASCII(::switches::kDisableFeatures);

  if (electron::fuses::IsDeviceBoundSessionsEnabled()) {
    // A production app that fused DBSC on must not be downgradable to mock
    // software keys by a command-line flag, so drop any request for them and
    // force the feature off.
    //
    // Only the local feature strings are touched. InitializeFeatureList() runs
    // twice in the browser process (ElectronMainDelegate::PreBrowserMain and
    // ElectronBrowserMainParts::PostEarlyInitialization), so rewriting the
    // process command line here would read back its own output and append the
    // disable entry a second time, leaking duplicates into app.commandLine and
    // into second-instance argv. It is also unnecessary: disable overrides win
    // in FeatureList, and child processes get their feature switches from the
    // FeatureList instance rather than from the browser's argv.
    std::vector<std::string_view> filtered_features;
    for (const auto& entry :
         base::FeatureList::SplitFeatureListString(enable_features)) {
      std::string name, study, group, params;
      if (base::FeatureList::ParseEnableFeatureString(entry, &name, &study,
                                                      &group, &params) &&
          name == unexportable_keys::
                      kEnableBoundSessionCredentialsSoftwareKeysForManualTesting
                          .name) {
        continue;
      }
      filtered_features.emplace_back(entry);
    }
    enable_features = base::JoinString(filtered_features, ",");
    disable_features +=
        std::string(",") +
        unexportable_keys::
            kEnableBoundSessionCredentialsSoftwareKeysForManualTesting.name;
  }

  // A renderer's command line depends on the WebContents it is created for,
  // so Electron warms one spare itself (for the first sandboxed window) rather
  // than letting content keep one alive at all times; apps that open many
  // windows can opt back into that with --enable-features.
  if (!std::ranges::any_of(
          base::FeatureList::SplitFeatureListString(enable_features),
          [](std::string_view entry) {
            std::string name, study, group, params;
            return base::FeatureList::ParseEnableFeatureString(
                       entry, &name, &study, &group, &params) &&
                   name == features::kSpareRendererForSitePerProcess.name;
          })) {
    disable_features +=
        std::string(",") + features::kSpareRendererForSitePerProcess.name;
  }
  disable_features +=
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
  // See https://chromium-review.googlesource.com/c/chromium/src/+/7204292
  // This feature causes the following sandbox failure on Windows:
  // sandbox\policy\win\sandbox_win.cc:777 Sandbox cannot access executable
  // electron.exe. Check filesystem permissions are valid.
  // See https://bit.ly/31yqMJR.: Access is denied. (0x5)
  disable_features +=
      std::string(",") + sandbox::policy::features::kNetworkServiceSandbox.name;
#endif

#if BUILDFLAG(ENABLE_PDF_VIEWER)
  // Enable window.showSaveFilePicker api for saving pdf files.
  // Refs https://issues.chromium.org/issues/373852607
  enable_features +=
      std::string(",") + chrome_pdf::features::kPdfUseShowSaveFilePicker.name;
#endif

#if BUILDFLAG(IS_LINUX)
  // Without this, globalShortcut is a silent no-op on GNOME Wayland (the
  // ozone factory returns no listener there). Chromium keeps it off due to
  // https://gitlab.gnome.org/GNOME/xdg-desktop-portal-gnome/-/issues/185,
  // but current GNOME persists bound shortcuts across sessions, so re-binds
  // are silent. A user-passed --disable-features for this still wins.
  enable_features +=
      std::string(",") + features::kGlobalShortcutsPortalPreferredTrigger.name;
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
