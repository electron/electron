// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_NODE_ENV_VARS_H_
#define ELECTRON_SHELL_COMMON_NODE_ENV_VARS_H_

#include <vector>

#include "base/strings/cstring_view.h"

namespace base {
class Environment;
}

namespace electron {

// Unsets the variables that are not respected when the nodeOptions fuse is
// disabled, and returns the ones that were set. Call in every process before
// Node.js is initialized.
std::vector<base::cstring_view> UnsetFuseGatedNodeEnvVars(
    base::Environment* env);

}  // namespace electron

#endif  // ELECTRON_SHELL_COMMON_NODE_ENV_VARS_H_
