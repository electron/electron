// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "electron/app/atom_main_delegate.h"

#include "base/mac/bundle_locations.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "brightray/common/application_info.h"
#include "brightray/common/mac/main_application_bundle.h"
#include "content/public/common/content_paths.h"

namespace electron {

namespace {

base::FilePath GetFrameworksPath() {
  return brightray::MainApplicationBundlePath().Append("Contents")
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

void ElectronMainDelegate::OverrideFrameworkBundlePath() {
  base::mac::SetOverrideFrameworkBundlePath(
      GetFrameworksPath().Append(ELECTRON_PRODUCT_NAME " Framework.framework"));
}

void ElectronMainDelegate::OverrideChildProcessPath() {
  base::FilePath frameworks_path = GetFrameworksPath();
  base::FilePath helper_path = GetHelperAppPath(frameworks_path,
                                                ELECTRON_PRODUCT_NAME);
  if (!base::PathExists(helper_path))
    helper_path = GetHelperAppPath(frameworks_path,
                                   brightray::GetApplicationName());
  if (!base::PathExists(helper_path))
    LOG(FATAL) << "Unable to find helper app";
  PathService::Override(content::CHILD_PROCESS_EXE, helper_path);
}

}  // namespace electron
