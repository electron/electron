// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/app/atom_main_delegate.h"

#include "atom/browser/mac/atom_application.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/mac/bundle_locations.h"
#include "base/mac/foundation_util.h"
#include "base/mac/scoped_nsautorelease_pool.h"
#include "base/path_service.h"
#include "base/strings/sys_string_conversions.h"
#include "brightray/common/application_info.h"
#include "brightray/common/mac/main_application_bundle.h"
#include "content/public/common/content_paths.h"

namespace atom {

namespace {

base::FilePath GetFrameworksPath() {
  return brightray::MainApplicationBundlePath()
      .Append("Contents")
      .Append("Frameworks");
}

base::FilePath GetHelperAppPath(const base::FilePath& frameworks_path,
                                const std::string& name) {
  return frameworks_path.Append(name + " Helper.app")
      .Append("Contents")
      .Append("MacOS")
      .Append(name + " Helper");
}

}  // namespace

void AtomMainDelegate::OverrideFrameworkBundlePath() {
  base::mac::SetOverrideFrameworkBundlePath(
      GetFrameworksPath().Append(ATOM_PRODUCT_NAME " Framework.framework"));
}

void AtomMainDelegate::OverrideChildProcessPath() {
  base::FilePath frameworks_path = GetFrameworksPath();
  base::FilePath helper_path =
      GetHelperAppPath(frameworks_path, ATOM_PRODUCT_NAME);
  if (!base::PathExists(helper_path))
    helper_path =
        GetHelperAppPath(frameworks_path, brightray::GetApplicationName());
  if (!base::PathExists(helper_path))
    LOG(FATAL) << "Unable to find helper app";
  base::PathService::Override(content::CHILD_PROCESS_EXE, helper_path);
}

void AtomMainDelegate::SetUpBundleOverrides() {
  base::mac::ScopedNSAutoreleasePool pool;
  NSBundle* bundle = brightray::MainApplicationBundle();
  std::string base_bundle_id =
      base::SysNSStringToUTF8([bundle bundleIdentifier]);
  NSString* team_id = [bundle objectForInfoDictionaryKey:@"ElectronTeamID"];
  if (team_id)
    base_bundle_id = base::SysNSStringToUTF8(team_id) + "." + base_bundle_id;
  base::mac::SetBaseBundleID(base_bundle_id.c_str());
}

void RegisterAtomCrApp() {
  // Force the NSApplication subclass to be used.
  [AtomApplication sharedApplication];
}

}  // namespace atom
