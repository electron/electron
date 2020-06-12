// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/app/electron_default_paths_mac.h"

#include "base/base_paths_mac.h"
#include "base/files/file_path.h"
#include "base/path_service.h"

#import <Cocoa/Cocoa.h>

namespace electron {

void GetMacAppLogsPath(base::FilePath* path) {
  NSString* bundle_name =
      [[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleName"];
  NSString* logs_path =
      [NSString stringWithFormat:@"Library/Logs/%@", bundle_name];
  NSString* library_path =
      [NSHomeDirectory() stringByAppendingPathComponent:logs_path];

  *path = base::FilePath([library_path UTF8String]);
}

}  // namespace electron
