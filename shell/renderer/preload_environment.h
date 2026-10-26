// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_RENDERER_PRELOAD_ENVIRONMENT_H_
#define ELECTRON_SHELL_RENDERER_PRELOAD_ENVIRONMENT_H_

#include <string_view>

#include "shell/common/api/api.mojom.h"
#include "v8/include/v8-forward.h"

namespace content {
class RenderFrame;
}

namespace gin_helper {
class Dictionary;
}

namespace electron {

class ServiceWorkerData;

// What sandboxed preload scripts see: the `electron` module, their `process`
// object and `require`, set up natively for a context without Node.js. Used
// by sandboxed renderers and by service worker preload realms.
namespace preload_environment {

enum class Flavor { kRenderer, kServiceWorker };

// The `electron` module: lazily loads each API's binding on first access.
v8::Local<v8::Object> CreateElectronModule(v8::Local<v8::Context> context,
                                           Flavor flavor);

// An EventEmitter carrying `base`'s properties, the process information the
// browser sent in `data`, and getProcessMemoryInfo(). It is also what
// EmitProcessEvent() emits on for `context` afterwards.
v8::Local<v8::Object> CreateProcessObject(
    v8::Local<v8::Context> context,
    gin_helper::Dictionary base,
    const mojom::RendererStartupDataPtr& data);

// Compiles and runs each preload script in `data` as
// (require, process, exports, module, global) => { ... }. A script that fails
// to load or throws is reported to the console and to the browser
// ('preload-error' on the WebContents); the others still run.
void RunPreloadScripts(v8::Local<v8::Context> context,
                       content::RenderFrame* render_frame,
                       ServiceWorkerData* service_worker_data,
                       v8::Local<v8::Object> process,
                       v8::Local<v8::Object> electron_module,
                       Flavor flavor,
                       const mojom::RendererStartupDataPtr& data);

// process.emit(event) for the process object created in `context`, if any.
void EmitProcessEvent(v8::Local<v8::Context> context, std::string_view event);

}  // namespace preload_environment

}  // namespace electron

#endif  // ELECTRON_SHELL_RENDERER_PRELOAD_ENVIRONMENT_H_
