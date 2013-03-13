// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Copyright (c) 2013 Adam Roben <adam@roben.org>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#import "main_delegate.h"

#include "common/application_name.h"
#include "base/mac/bundle_locations.h"
#include "base/mac/foundation_util.h"
#include "base/path_service.h"
#include "content/public/common/content_paths.h"

namespace brightray {

namespace {

base::FilePath GetFrameworksPath() {
  // Start out with the path to the running executable.
  base::FilePath path;
  PathService::Get(base::FILE_EXE, &path);
  
  // Up to Contents.
  if (base::mac::IsBackgroundOnlyProcess()) {
    // The running executable is the helper. Go up five steps:
    // Contents/Frameworks/Helper.app/Contents/MacOS/Helper
    // ^ to here                                     ^ from here
    path = path.DirName().DirName().DirName().DirName().DirName();
  } else {
    // One step up to MacOS, another to Contents.
    path = path.DirName().DirName();
  }
  DCHECK_EQ(path.BaseName().value(), "Contents");
  
  // Go into the frameworks directory.
  return path.Append("Frameworks");
}

}

void MainDelegate::OverrideFrameworkBundlePath() {
  base::FilePath helper_path = GetFrameworksPath().Append(GetApplicationName() + ".framework");

  base::mac::SetOverrideFrameworkBundlePath(helper_path);
}

void MainDelegate::OverrideChildProcessPath() {
  base::FilePath helper_path = GetFrameworksPath().Append(GetApplicationName() + " Helper.app")
    .Append("Contents")
    .Append("MacOS")
    .Append(GetApplicationName() + " Helper");

  PathService::Override(content::CHILD_PROCESS_EXE, helper_path);
}

}
