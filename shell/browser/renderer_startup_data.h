// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_RENDERER_STARTUP_DATA_H_
#define ELECTRON_SHELL_BROWSER_RENDERER_STARTUP_DATA_H_

#include "shell/browser/preload_script.h"
#include "shell/common/api/api.mojom.h"

class GURL;

namespace content {
class BrowserContext;
}  // namespace content

namespace electron::renderer_startup_data {

// Builds a RendererStartupData for the given session containing the registered
// preload scripts of |type| (kWebFrame for the per-frame ElectronFrameStartup
// push, or kServiceWorker for the GetServiceWorkerStartupData blob), the
// browser env snapshot, and process.helperExecPath. Reads preload contents
// synchronously — call behind a ScopedAllowBlockingForElectron.
//
// |consumer_site| is the site of the document that will consume the data
// (preload_code_cache::SiteForFrame()); cached preload bytecode is only
// attached when it was produced by a renderer of the same site. Pass an
// empty GURL for realms with no code cache (service workers).
mojom::RendererStartupDataPtr Build(content::BrowserContext* browser_context,
                                    PreloadScript::ScriptType type,
                                    const GURL& consumer_site);

}  // namespace electron::renderer_startup_data

#endif  // ELECTRON_SHELL_BROWSER_RENDERER_STARTUP_DATA_H_
