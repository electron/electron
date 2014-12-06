// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/app/atom_main_delegate.h"

#include "base/mac/bundle_locations.h"
#include "base/files/file_path.h"
#include "base/path_service.h"
#include "brightray/common/mac/main_application_bundle.h"
#include "content/public/common/content_paths.h"

namespace atom {

namespace {

base::FilePath GetFrameworksPath() {
  return brightray::MainApplicationBundlePath().Append("Contents")
                                               .Append("Frameworks");
}

std::string GetApplicationName() {
  std::string name = brightray::MainApplicationBundlePath().BaseName().AsUTF8Unsafe();
  return name.substr(0, name.length() - 4/*.app*/);
}

}  // namespace

void AtomMainDelegate::OverrideFrameworkBundlePath() {
  base::FilePath bundlePath = GetFrameworksPath();
  std::string app_name = GetApplicationName();

  base::mac::SetOverrideFrameworkBundlePath(bundlePath
      .Append(app_name + " Framework.framework"));
}

void AtomMainDelegate::OverrideChildProcessPath() {
  std::string app_name = GetApplicationName();

  base::FilePath helper_path = GetFrameworksPath().Append(app_name + " Helper.app")
                                                  .Append("Contents")
                                                  .Append("MacOS")
                                                  .Append(app_name + " Helper");
  PathService::Override(content::CHILD_PROCESS_EXE, helper_path);
}

}  // namespace atom
