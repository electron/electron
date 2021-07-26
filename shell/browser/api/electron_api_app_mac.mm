// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <string>

#include "base/path_service.h"
#include "shell/browser/api/electron_api_app.h"
#include "shell/common/electron_paths.h"
#include "shell/common/node_includes.h"
#include "shell/common/process_util.h"

#import <Cocoa/Cocoa.h>
#import <sys/sysctl.h>

namespace electron {

namespace api {

void App::SetAppLogsPath(gin_helper::ErrorThrower thrower,
                         absl::optional<base::FilePath> custom_path) {
  if (custom_path.has_value()) {
    if (!custom_path->IsAbsolute()) {
      thrower.ThrowError("Path must be absolute");
      return;
    }
    {
      base::ThreadRestrictions::ScopedAllowIO allow_io;
      base::PathService::Override(DIR_APP_LOGS, custom_path.value());
    }
  } else {
    NSString* bundle_name =
        [[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleName"];
    NSString* logs_path =
        [NSString stringWithFormat:@"Library/Logs/%@", bundle_name];
    NSString* library_path =
        [NSHomeDirectory() stringByAppendingPathComponent:logs_path];
    {
      base::ThreadRestrictions::ScopedAllowIO allow_io;
      base::PathService::Override(DIR_APP_LOGS,
                                  base::FilePath([library_path UTF8String]));
    }
  }
}

void App::SetActivationPolicy(gin_helper::ErrorThrower thrower,
                              const std::string& policy) {
  NSApplicationActivationPolicy activation_policy;
  if (policy == "accessory") {
    activation_policy = NSApplicationActivationPolicyAccessory;
  } else if (policy == "prohibited") {
    activation_policy = NSApplicationActivationPolicyProhibited;
  } else if (policy == "regular") {
    activation_policy = NSApplicationActivationPolicyRegular;
  } else {
    thrower.ThrowError("Invalid activation policy: must be one of 'regular', "
                       "'accessory', or 'prohibited'");
    return;
  }

  [NSApp setActivationPolicy:activation_policy];
}

bool App::IsRunningUnderRosettaTranslation() const {
  node::Environment* env =
      node::Environment::GetCurrent(JavascriptEnvironment::GetIsolate());

  EmitWarning(env,
              "The app.runningUnderRosettaTranslation API is deprecated, use "
              "app.runningUnderARM64Translation instead.",
              "electron");
  return IsRunningUnderARM64Translation();
}

bool App::IsRunningUnderARM64Translation() const {
  int proc_translated = 0;
  size_t size = sizeof(proc_translated);
  if (sysctlbyname("sysctl.proc_translated", &proc_translated, &size, NULL,
                   0) == -1) {
    return false;
  }
  return proc_translated == 1;
}

}  // namespace api

}  // namespace electron
