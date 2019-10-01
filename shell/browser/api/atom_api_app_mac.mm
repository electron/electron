// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "base/path_service.h"
#include "shell/browser/api/atom_api_app.h"
#include "shell/browser/atom_paths.h"
#include "shell/common/native_mate_converters/file_path_converter.h"

#import <Cocoa/Cocoa.h>

namespace electron {

namespace api {

void App::SetAppLogsPath(base::Optional<base::FilePath> custom_path,
                         mate::Arguments* args) {
  if (custom_path.has_value()) {
    if (!custom_path->IsAbsolute()) {
      args->ThrowError("Path must be absolute");
      return;
    }
    base::PathService::Override(DIR_APP_LOGS, custom_path.value());
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

}  // namespace api

}  // namespace electron
