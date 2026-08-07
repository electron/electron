// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_RENDERER_STARTUP_DATA_H_
#define ELECTRON_SHELL_BROWSER_RENDERER_STARTUP_DATA_H_

#include "shell/browser/preload_script.h"
#include "shell/common/api/api.mojom.h"

namespace content {
class BrowserContext;
class RenderFrameHost;
}  // namespace content

namespace electron::renderer_startup_data {

// Builds a RendererStartupData for the given session containing the registered
// preload scripts of |type| (kServiceWorker for the GetServiceWorkerStartupData
// blob), the browser env snapshot, and process.helperExecPath. No preload code
// cache is attached — that only exists for frame realms. Reads preload
// contents synchronously — call behind a ScopedAllowBlockingForElectron.
mojom::RendererStartupDataPtr Build(content::BrowserContext* browser_context,
                                    PreloadScript::ScriptType type);

// Builds the RendererStartupData pushed to a frame: session-registered
// kWebFrame preloads (in registration order) followed by the per-WebContents
// webPreferences.preload — the same order as the legacy BROWSER_SANDBOX_LOAD
// handler's getPreloadScriptsFromEvent(). Attaches any preload code cache
// previously produced for |rfh|'s site, and records the served (id, hash)
// tuples with preload_code_cache so SetPreloadCodeCache writes from this
// frame can be validated against exactly what was served. Reads preload
// contents synchronously — call behind a ScopedAllowBlockingForElectron.
mojom::RendererStartupDataPtr BuildForFrame(content::RenderFrameHost* rfh);

}  // namespace electron::renderer_startup_data

#endif  // ELECTRON_SHELL_BROWSER_RENDERER_STARTUP_DATA_H_
