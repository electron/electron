// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_RENDERER_PRELOAD_UTILS_H_
#define ELECTRON_SHELL_RENDERER_PRELOAD_UTILS_H_

#include "shell/common/api/api.mojom.h"
#include "v8/include/v8-forward.h"

namespace gin_helper {
class Arguments;
}

namespace electron::preload_utils {

v8::Local<v8::Value> GetBinding(v8::Isolate* isolate,
                                v8::Local<v8::String> key);

v8::Local<v8::Value> CreatePreloadScript(v8::Isolate* isolate,
                                         v8::Local<v8::String> source);

double Uptime();

// Converts the startup data the browser pushed (via ElectronFrameStartup or
// ElectronWorkerStartup) into the `{ preloadScripts, process }` shape that
// lib/sandboxed_renderer/init.ts and lib/preload_realm/init.ts expect — the
// same shape the legacy BROWSER_SANDBOX_LOAD sync IPC returned. Returns an
// empty MaybeLocal when |data| is null.
v8::MaybeLocal<v8::Value> BuildStartupData(
    v8::Isolate* isolate,
    const mojom::RendererStartupDataPtr& data);

}  // namespace electron::preload_utils

#endif  // ELECTRON_SHELL_RENDERER_PRELOAD_UTILS_H_
