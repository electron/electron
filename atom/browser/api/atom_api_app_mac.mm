// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_app.h"
#include "atom/browser/atom_paths.h"
#include "base/path_service.h"

#import <Cocoa/Cocoa.h>

namespace atom {

namespace api {

void App::SetAppLogsPath() {
  base::FilePath path;
  NSString* bundleName =
      [[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleName"];
  NSString* logsPath =
      [NSString stringWithFormat:@"Library/Logs/%@", bundleName];
  NSString* libraryPath =
      [NSHomeDirectory() stringByAppendingPathComponent:logsPath];

  base::PathService::Override(DIR_APP_LOGS,
                              base::FilePath([libraryPath UTF8String]));
}

}  // namespace atom

}  // namespace api
