// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_RENDERER_PRELOAD_UTILS_H_
#define ELECTRON_SHELL_RENDERER_PRELOAD_UTILS_H_

#include <string>
#include <vector>

#include "shell/common/api/api.mojom.h"
#include "v8/include/v8-forward.h"

namespace content {
class RenderFrame;
}

namespace electron {
class ServiceWorkerData;
}

namespace electron::preload_utils {

v8::Local<v8::Value> GetBinding(v8::Isolate* isolate,
                                v8::Local<v8::String> key);

// Compiles a preload script's body via v8::ScriptCompiler::CompileFunction()
// with |param_names| as the function's parameter list, and returns the
// resulting Function. The preload contents are looked up by |script_id| from
// the per-frame (|render_frame|) or per-worker (|service_worker_data|) mojo-
// cached startup data — they never become a V8 string until the single copy
// here for the compile, and the V8 code cache is consumed/produced entirely
// on the C++ side (BufferNotOwned / BigBuffer). Exactly one of
// |render_frame| / |service_worker_data| should be non-null. Only the frame
// path produces and ships a cache (the worker realm has no per-frame channel).
v8::Local<v8::Value> CreatePreloadScript(
    content::RenderFrame* render_frame,
    ServiceWorkerData* service_worker_data,
    v8::Isolate* isolate,
    const std::string& script_id,
    const std::vector<std::string>& param_names);

double Uptime();

// Converts the startup data the browser delivered (via ElectronFrameStartup
// for frames, or EmbeddedWorkerStartParams for service workers) into the
// `{ preloadScripts, process }` shape that
// lib/sandboxed_renderer/init.ts and lib/preload_realm/init.ts expect — the
// same shape the legacy BROWSER_SANDBOX_LOAD sync IPC returned. Returns an
// empty MaybeLocal when |data| is null.
v8::MaybeLocal<v8::Value> BuildStartupData(
    v8::Isolate* isolate,
    const mojom::RendererStartupDataPtr& data);

}  // namespace electron::preload_utils

#endif  // ELECTRON_SHELL_RENDERER_PRELOAD_UTILS_H_
