// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/node_env_vars.h"

#include "base/containers/span.h"
#include "base/environment.h"
#include "base/strings/cstring_view.h"
#include "electron/fuses.h"

namespace electron {

namespace {

constexpr base::cstring_view kFuseGated[] = {
    "NODE_COMPILE_CACHE",          "NODE_COMPILE_CACHE_PORTABLE",
    "NODE_COMPILE_CACHE_READONLY", "NODE_EXTRA_CA_CERTS",
    "NODE_REPL_EXTERNAL_MODULE",
};

// Except for NODE_TLS_REJECT_UNAUTHORIZED, these are the variables read by
// HandleEnvOptions() in Node.js' src/node_options.cc, which does not check
// kDisableNodeOptionsEnv.
constexpr base::cstring_view kInherited[] = {
    "NODE_PENDING_DEPRECATION",     "NODE_PRESERVE_SYMLINKS",
    "NODE_PRESERVE_SYMLINKS_MAIN",  "NODE_REDIRECT_WARNINGS",
    "NODE_TLS_REJECT_UNAUTHORIZED", "NODE_USE_ENV_PROXY",
    "NODE_USE_SYSTEM_CA",
};

std::vector<base::cstring_view> UnsetVars(
    base::Environment* env,
    base::span<const base::cstring_view> names) {
  std::vector<base::cstring_view> unset;
  if (fuses::IsNodeOptionsEnabled())
    return unset;

  for (base::cstring_view name : names) {
    if (env->HasVar(name)) {
      env->UnSetVar(name);
      unset.push_back(name);
    }
  }
  return unset;
}

}  // namespace

std::vector<base::cstring_view> UnsetFuseGatedNodeEnvVars(
    base::Environment* env) {
  return UnsetVars(env, kFuseGated);
}

void UnsetInheritedNodeEnvVars(base::Environment* env) {
  UnsetVars(env, kInherited);
}

}  // namespace electron
