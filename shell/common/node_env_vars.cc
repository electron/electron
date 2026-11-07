// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/node_env_vars.h"

#include "base/environment.h"
#include "base/strings/cstring_view.h"
#include "electron/fuses.h"

namespace electron {

namespace {

constexpr base::cstring_view kFuseGated[] = {
    "NODE_COMPILE_CACHE",
    "NODE_COMPILE_CACHE_PORTABLE",
    "NODE_COMPILE_CACHE_READONLY",
    "NODE_EXTRA_CA_CERTS",
};

}  // namespace

std::vector<base::cstring_view> UnsetFuseGatedNodeEnvVars(
    base::Environment* env) {
  std::vector<base::cstring_view> unset;
  if (fuses::IsNodeOptionsEnabled())
    return unset;

  for (base::cstring_view name : kFuseGated) {
    if (env->HasVar(name)) {
      env->UnSetVar(name);
      unset.push_back(name);
    }
  }
  return unset;
}

}  // namespace electron
