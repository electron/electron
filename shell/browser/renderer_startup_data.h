// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_RENDERER_STARTUP_DATA_H_
#define ELECTRON_SHELL_BROWSER_RENDERER_STARTUP_DATA_H_

#include "shell/browser/preload_script.h"
#include "shell/common/api/api.mojom.h"

class GURL;

namespace url {
class Origin;
}

namespace content {
class BrowserContext;
class RenderFrameHost;
}  // namespace content

namespace electron::renderer_startup_data {

// Builds a RendererStartupData for the given session containing the registered
// preload scripts of |type| whose URL filters match |url| (kServiceWorker and
// the worker's scope for the GetServiceWorkerStartupData blob), the browser env
// snapshot, and process.helperExecPath. No preload code cache is attached —
// that only exists for frame realms. Reads preload contents synchronously —
// call behind a ScopedAllowBlockingForElectron.
mojom::RendererStartupDataPtr Build(content::BrowserContext* browser_context,
                                    PreloadScript::ScriptType type,
                                    const GURL& url);

// Builds the RendererStartupData pushed to a frame for a document at |url|
// with security origin |origin|: session-registered kWebFrame preloads whose
// URL filters match the document (in registration order) followed by the
// per-WebContents webPreferences.preload — the same order as the legacy
// BROWSER_SANDBOX_LOAD handler's getPreloadScriptsFromEvent(). Attaches any
// preload code cache previously produced for |rfh|'s site, and records the
// served (id, hash) tuples with preload_code_cache so SetPreloadCodeCache
// writes from this frame can be validated against exactly what was served.
// Reads preload contents synchronously — call behind a
// ScopedAllowBlockingForElectron.
// For a frame without the sandbox (Node.js integration) only the preload
// paths are filled in: that renderer loads the files itself.
mojom::RendererStartupDataPtr BuildForFrame(content::RenderFrameHost* rfh,
                                            const GURL& url,
                                            const url::Origin& origin);

// Whether |rfh| lives in a sandboxed renderer process (sandbox bundle) rather
// than one with Node.js integration.
bool IsFrameInSandboxedRenderer(content::RenderFrameHost* rfh);

}  // namespace electron::renderer_startup_data

#endif  // ELECTRON_SHELL_BROWSER_RENDERER_STARTUP_DATA_H_
