// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_app.h"
#include "atom/browser/atom_paths.h"
#include "atom/common/native_mate_converters/file_path_converter.h"
#include "base/path_service.h"

#import <Cocoa/Cocoa.h>

namespace atom {

namespace api {

void App::SetAppLogsPath(mate::Arguments* args) {
  base::FilePath custom_path;
  if (args->GetNext(&custom_path)) {
    if (!custom_path.IsAbsolute()) {
      args->ThrowError("Path must be absolute");
      return;
    }
    base::PathService::Override(DIR_APP_LOGS, custom_path);
  } else {
    NSString* bundle_name =
        [[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleName"];
    NSString* logs_path =
        [NSString stringWithFormat:@"Library/Logs/%@", bundle_name];
    NSString* library_path =
        [NSHomeDirectory() stringByAppendingPathComponent:logs_path];
    base::PathService::Override(DIR_APP_LOGS,
                                base::FilePath([library_path UTF8String]));
  }
}

}  // namespace atom

}  // namespace api
