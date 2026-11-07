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

// Both functions do nothing unless the nodeOptions fuse is disabled.

// Unsets the variables that are never respected. Call in every process before
// Node.js is initialized. Returns the variables that were set.
std::vector<base::cstring_view> UnsetFuseGatedNodeEnvVars(
    base::Environment* env);

// Unsets the variables that are only respected when the app sets them itself.
// Call at startup, before app code runs and before any child process exists.
// Children must not call this, so that they keep what the app passes to them.
void UnsetInheritedNodeEnvVars(base::Environment* env);

}  // namespace electron

#endif  // ELECTRON_SHELL_COMMON_NODE_ENV_VARS_H_
