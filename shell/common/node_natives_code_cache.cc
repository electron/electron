// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/node_natives_code_cache.h"

#include "shell/browser/javascript_environment.h"
#include "shell/common/node_includes.h"
#include "shell/common/node_natives_code_cache_internal.h"
#include "shell/common/process_util.h"
#include "third_party/electron_node/src/node_snapshot_builder.h"

namespace electron {

Js2cCacheFlavor CurrentProcessJs2cCacheFlavor(bool has_node_env) {
  if (IsBrowserProcess() || IsUtilityProcess()) {
    // Processes that host Node through a JavascriptEnvironment: the browser
    // process, ELECTRON_RUN_AS_NODE children (no --type either) and the node
    // service's utility process. On builds that embed a Node startup snapshot
    // their isolate normally comes from that snapshot, which is what the
    // browser flavor is keyed to there. When such a process nevertheless boots
    // from the v8 context snapshot (a custom V8 snapshot is loaded), its
    // isolate and flag set match the utility flavor instead (which then lacks
    // browser_init; it compiles from source). Without an embedded snapshot both
    // flavors are keyed to the v8 context snapshot and split by init bundle.
    if (JavascriptEnvironment::NodeSnapshot())
      return Js2cCacheFlavor::kBrowser;
    const bool build_embeds_node_snapshot =
        node::SnapshotBuilder::GetEmbeddedSnapshotData() != nullptr;
    return IsUtilityProcess() || build_embeds_node_snapshot
               ? Js2cCacheFlavor::kUtility
               : Js2cCacheFlavor::kBrowser;
  }
  if (IsRendererProcess()) {
    // No Node env means the V8 flag set stays frozen -- a distinct flavor.
    return has_node_env ? Js2cCacheFlavor::kRenderer
                        : Js2cCacheFlavor::kSandbox;
  }
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
