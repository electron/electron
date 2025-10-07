// Copyright (c) 2025 Salesforce, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_RENDERER_PRELOAD_REALM_CONTEXT_H_
#define ELECTRON_SHELL_RENDERER_PRELOAD_REALM_CONTEXT_H_

#include "v8/include/v8-forward.h"

namespace electron {
class ServiceWorkerData;
}

namespace electron::preload_realm {

// Get initiator context given the preload context.
v8::MaybeLocal<v8::Context> GetInitiatorContext(v8::Local<v8::Context> context,
                                                v8::Isolate* target_isolate);

// Get the preload context given the initiator context.
v8::MaybeLocal<v8::Context> GetPreloadRealmContext(
    v8::Local<v8::Context> context);

// Get service worker data given the preload realm context.
electron::ServiceWorkerData* GetServiceWorkerData(
    v8::Local<v8::Context> context);

// Create
void OnCreatePreloadableV8Context(
    v8::Isolate* const isolate,
    v8::Local<v8::Context> initiator_context,
    electron::ServiceWorkerData* service_worker_data);

}  // namespace electron::preload_realm

#endif  // ELECTRON_SHELL_RENDERER_PRELOAD_REALM_CONTEXT_H_
