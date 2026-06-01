// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

// Used when electron_generate_js2c_code_cache=false; all flavors return an
// empty set so the BuiltinLoader compiles from source, as before.

#include "shell/common/node_natives_code_cache_internal.h"

#include "base/no_destructor.h"
#include "shell/common/node_includes.h"

namespace electron::internal {

static const std::vector<node::builtins::CodeCacheInfo>& Empty() {
  static const base::NoDestructor<std::vector<node::builtins::CodeCacheInfo>>
      kEmpty;
  return *kEmpty;
}

const std::vector<node::builtins::CodeCacheInfo>& Js2cCacheSandbox() {
  return Empty();
}
const std::vector<node::builtins::CodeCacheInfo>& Js2cCacheRenderer() {
  return Empty();
}
const std::vector<node::builtins::CodeCacheInfo>& Js2cCacheBrowser() {
  return Empty();
}
const std::vector<node::builtins::CodeCacheInfo>& Js2cCacheUtility() {
  return Empty();
}
const std::vector<node::builtins::CodeCacheInfo>& Js2cCacheWorker() {
  return Empty();
}

}  // namespace electron::internal
