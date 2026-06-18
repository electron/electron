// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/node_natives_code_cache.h"

#include "shell/common/node_natives_code_cache_internal.h"
#include "shell/common/process_util.h"

namespace electron {

Js2cCacheFlavor CurrentProcessJs2cCacheFlavor(bool has_node_env) {
  if (IsBrowserProcess())
    return Js2cCacheFlavor::kBrowser;
  if (IsRendererProcess()) {
    // No Node env means the V8 flag set stays frozen -- a distinct flavor.
    return has_node_env ? Js2cCacheFlavor::kRenderer
                        : Js2cCacheFlavor::kSandbox;
  }
  if (IsUtilityProcess())
    return Js2cCacheFlavor::kUtility;
  return Js2cCacheFlavor::kWorker;
}

const std::vector<node::builtins::CodeCacheInfo>& GetNativesCodeCache(
    Js2cCacheFlavor flavor) {
  switch (flavor) {
    case Js2cCacheFlavor::kSandbox:
      return internal::Js2cCacheSandbox();
    case Js2cCacheFlavor::kRenderer:
      return internal::Js2cCacheRenderer();
    case Js2cCacheFlavor::kBrowser:
      return internal::Js2cCacheBrowser();
    case Js2cCacheFlavor::kUtility:
      return internal::Js2cCacheUtility();
    case Js2cCacheFlavor::kWorker:
      return internal::Js2cCacheWorker();
  }
}

}  // namespace electron
