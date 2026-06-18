// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_NODE_NATIVES_CODE_CACHE_H_
#define ELECTRON_SHELL_COMMON_NODE_NATIVES_CODE_CACHE_H_

#include <vector>

namespace node::builtins {
struct CodeCacheInfo;
}  // namespace node::builtins

namespace electron {

// V8's code cache key includes the non-default flag set and the snapshot's
// read-only checksum, both of which differ per process type, so one cache
// variant is generated per flavor and each process consumes the matching one.
enum class Js2cCacheFlavor {
  kSandbox,   // sandboxed renderer (no Node env)
  kRenderer,  // normal renderer
  kBrowser,
  kUtility,
  kWorker,
};

// `has_node_env` distinguishes a sandboxed renderer (false) from a normal
// one (true): pass `node::Environment::GetCurrent(context) != nullptr`.
Js2cCacheFlavor CurrentProcessJs2cCacheFlavor(bool has_node_env = true);

// Build-time V8 code cache for the embedded electron/js2c/* bundles, fed to
// the BuiltinLoader so they are deserialized rather than parsed + compiled
// from source on startup. Empty when no generated cache was built; V8 also
// gracefully falls back to compiling from source on a stale/mismatched blob.
const std::vector<node::builtins::CodeCacheInfo>& GetNativesCodeCache(
    Js2cCacheFlavor flavor);

}  // namespace electron

#endif  // ELECTRON_SHELL_COMMON_NODE_NATIVES_CODE_CACHE_H_
