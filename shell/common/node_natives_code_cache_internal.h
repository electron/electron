// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_NODE_NATIVES_CODE_CACHE_INTERNAL_H_
#define ELECTRON_SHELL_COMMON_NODE_NATIVES_CODE_CACHE_INTERNAL_H_

#include <vector>

namespace node::builtins {
struct CodeCacheInfo;
}  // namespace node::builtins

namespace electron::internal {

// Defined by the generated electron_natives_code_cache_<flavor>.cc when
// electron_generate_js2c_code_cache, otherwise by the empty stub.
const std::vector<node::builtins::CodeCacheInfo>& Js2cCacheSandbox();
const std::vector<node::builtins::CodeCacheInfo>& Js2cCacheRenderer();
const std::vector<node::builtins::CodeCacheInfo>& Js2cCacheBrowser();
const std::vector<node::builtins::CodeCacheInfo>& Js2cCacheUtility();
const std::vector<node::builtins::CodeCacheInfo>& Js2cCacheWorker();

}  // namespace electron::internal

#endif  // ELECTRON_SHELL_COMMON_NODE_NATIVES_CODE_CACHE_INTERNAL_H_
