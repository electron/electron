// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/electron_api_app.h"

#include <string>

#include "base/path_service.h"
#include "shell/common/electron_paths.h"
#include "shell/common/node_includes.h"
#include "shell/common/process_util.h"
#include "shell/common/thread_restrictions.h"

#import <Cocoa/Cocoa.h>
#import <sys/sysctl.h>

namespace electron::api {

base::FilePath App::GetDefaultAppLogPath() {
  NSString* bundle_name =
      [[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleName"];
  NSString* logs_path =
      [NSString stringWithFormat:@"Library/Logs/%@", bundle_name];
  NSString* library_path =
      [NSHomeDirectory() stringByAppendingPathComponent:logs_path];
  return base::FilePath{[library_path UTF8String]};
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

bool App::IsRunningUnderARM64Translation() const {
  int proc_translated = 0;
  size_t size = sizeof(proc_translated);
  if (sysctlbyname("sysctl.proc_translated", &proc_translated, &size, nullptr,
                   0) == -1) {
    return false;
  }
  return proc_translated == 1;
}

}  // namespace electron::api
