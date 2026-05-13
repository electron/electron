// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_RENDERER_STARTUP_DATA_H_
#define ELECTRON_SHELL_BROWSER_RENDERER_STARTUP_DATA_H_

#include "shell/browser/preload_script.h"
#include "shell/common/api/api.mojom.h"

namespace content {
class BrowserContext;
class RenderProcessHost;
}  // namespace content

namespace electron::renderer_startup_data {

// Builds a RendererStartupData for the given session containing the registered
// preload scripts of |type| (e.g. kWebFrame for the per-frame push, or
// kServiceWorker for the per-process worker push), the browser env snapshot,
// and process.helperExecPath. Reads preload contents synchronously — call
// behind a ScopedAllowBlockingForElectron. Used by both the per-frame
// ElectronFrameStartup push and the per-process ElectronWorkerStartup push.
mojom::RendererStartupDataPtr Build(content::BrowserContext* browser_context,
                                    PreloadScript::ScriptType type);

// Pushes the service-worker startup data for |host|'s session over the
// per-process ElectronWorkerStartup associated channel. No-op if the session
// has no registered service-worker preload scripts. Safe to call repeatedly.
void SendToProcess(content::RenderProcessHost* host);

}  // namespace electron::renderer_startup_data

#endif  // ELECTRON_SHELL_BROWSER_RENDERER_STARTUP_DATA_H_
