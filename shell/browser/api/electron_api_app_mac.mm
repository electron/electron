// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <string>

#include "base/path_service.h"
#include "shell/browser/api/electron_api_app.h"
#include "shell/common/electron_paths.h"

#import <Cocoa/Cocoa.h>

namespace electron {

namespace api {

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

}  // namespace api

}  // namespace electron
