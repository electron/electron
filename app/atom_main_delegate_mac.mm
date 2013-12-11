// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "app/atom_main_delegate.h"

#import "base/mac/bundle_locations.h"
#import "base/mac/foundation_util.h"
#import "base/mac/mac_util.h"
#include "base/command_line.h"
#include "base/path_service.h"
#include "base/strings/sys_string_conversions.h"
#include "content/public/common/content_paths.h"
#include "content/public/common/content_switches.h"
#include "vendor/brightray/common/application_info.h"
#include "vendor/brightray/common/mac/main_application_bundle.h"

namespace atom {

namespace {

base::FilePath GetFrameworksPath() {
  return brightray::MainApplicationBundlePath().Append("Contents")
                                               .Append("Frameworks");
}

}  // namespace

base::FilePath AtomMainDelegate::GetResourcesPakFilePath() {
  NSString* path = [base::mac::FrameworkBundle()
      pathForResource:@"content_shell" ofType:@"pak"];
  return base::mac::NSStringToFilePath(path);
}

void AtomMainDelegate::OverrideFrameworkBundlePath() {
  base::mac::SetOverrideFrameworkBundlePath(
      GetFrameworksPath().Append("Atom Framework.framework"));
}

void AtomMainDelegate::OverrideChildProcessPath() {
  base::FilePath helper_path = GetFrameworksPath().Append("Atom Helper.app")
                                                  .Append("Contents")
                                                  .Append("MacOS")
                                                  .Append("Atom Helper");
  PathService::Override(content::CHILD_PROCESS_EXE, helper_path);
}

}  // namespace atom
