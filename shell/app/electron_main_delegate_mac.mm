// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/app/electron_main_delegate.h"

#include <string>

#include "base/apple/bundle_locations.h"
#include "base/apple/foundation_util.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/strings/sys_string_conversions.h"
#include "content/browser/mac_helpers.h"
#include "content/public/common/content_paths.h"
#include "shell/browser/electron_child_process_host_flags.h"
#include "shell/browser/mac/electron_application.h"
#include "shell/common/application_info.h"
#include "shell/common/mac/main_application_bundle.h"

namespace electron {

namespace {

base::FilePath GetFrameworksPath() {
  return MainApplicationBundlePath().Append("Contents").Append("Frameworks");
}

// The helper apps live inside the framework's versioned bundle directory, e.g.
// Contents/Frameworks/Electron Framework.framework/Versions/<version>/Helpers/.
// The framework's top-level "Helpers" symlink points into Versions/Current, so
// resolving through the framework bundle keeps this version-agnostic.
base::FilePath GetHelpersPath() {
  return GetFrameworksPath()
      .Append(ELECTRON_PRODUCT_NAME " Framework.framework")
      .Append("Helpers");
}

base::FilePath GetHelperAppPath(const base::FilePath& helpers_path,
                                const std::string& name) {
  // Figure out what helper we are running
  base::FilePath path;
  base::PathService::Get(base::FILE_EXE, &path);

  std::string helper_name = "Helper";
  if (const auto& val = path.value();
      val.ends_with(content::kMacHelperSuffix_renderer)) {
    helper_name += content::kMacHelperSuffix_renderer;
  } else if (val.ends_with(content::kMacHelperSuffix_gpu)) {
    helper_name += content::kMacHelperSuffix_gpu;
  } else if (val.ends_with(kElectronMacHelperSuffixPlugin)) {
    helper_name += kElectronMacHelperSuffixPlugin;
  }

  return helpers_path.Append(name + " " + helper_name + ".app")
      .Append("Contents")
      .Append("MacOS")
      .Append(name + " " + helper_name);
}

}  // namespace

void ElectronMainDelegate::OverrideFrameworkBundlePath() {
  base::apple::SetOverrideFrameworkBundlePath(
      GetFrameworksPath().Append(ELECTRON_PRODUCT_NAME " Framework.framework"));
}

void ElectronMainDelegate::OverrideChildProcessPath() {
  base::FilePath helpers_path = GetHelpersPath();
  base::FilePath helper_path =
      GetHelperAppPath(helpers_path, ELECTRON_PRODUCT_NAME);
  if (!base::PathExists(helper_path))
    helper_path = GetHelperAppPath(helpers_path, GetApplicationName());
  if (!base::PathExists(helper_path))
    LOG(FATAL) << "Unable to find helper app";
  // The helper lives inside the framework's versioned bundle directory, reached
  // via the framework's top-level "Helpers" symlink (which points through
  // Versions/Current). Resolve the symlinks to the real versioned path so it
  // matches the canonicalized program path that content computes via
  // base::MakeAbsoluteFilePath — otherwise the safety check in
  // ElectronBrowserClient::AppendExtraCommandLineSwitches would compare the
  // symlinked path against the resolved path and abort the launch.
  if (base::FilePath resolved = base::MakeAbsoluteFilePath(helper_path);
      !resolved.empty()) {
    helper_path = std::move(resolved);
  }
  base::PathService::OverrideAndCreateIfNeeded(
      content::CHILD_PROCESS_EXE, helper_path, /*is_absolute=*/true,
      /*create=*/false);
}

void ElectronMainDelegate::SetUpBundleOverrides() {
  @autoreleasepool {
    NSBundle* bundle = MainApplicationBundle();
    std::string base_bundle_id =
        base::SysNSStringToUTF8([bundle bundleIdentifier]);
    NSString* team_id = [bundle objectForInfoDictionaryKey:@"ElectronTeamID"];
    if (team_id)
      base_bundle_id = base::SysNSStringToUTF8(team_id) + "." + base_bundle_id;
    base::apple::SetBaseBundleIDOverride(base_bundle_id);
  }
}

void RegisterAtomCrApp() {
  // Force the NSApplication subclass to be used.
  [AtomApplication sharedApplication];
}

}  // namespace electron
