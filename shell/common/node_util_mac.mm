// Copyright (c) 2023 Microsoft, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/node_util.h"

#include <Foundation/Foundation.h>

namespace electron::util {

bool UnsetAllNodeEnvs() {
  bool has_unset = false;
  for (NSString* env in NSProcessInfo.processInfo.environment) {
    if (![env hasPrefix:@"NODE_"])
      continue;
    const char* name = [[env componentsSeparatedByString:@"="][0] UTF8String];
    unsetenv(name);
    has_unset = true;
  }
  return has_unset;
}

}  // namespace electron::util
