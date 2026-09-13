// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_GIN_HELPER_NODE_EVENT_EMITTER_H_
#define ELECTRON_SHELL_COMMON_GIN_HELPER_NODE_EVENT_EMITTER_H_

#include "v8/include/v8-forward.h"

namespace gin_helper {

// Creates a native implementation of Node.js's `EventEmitter` class for
// `context`, for use in script contexts that have no Node.js environment
// (sandboxed renderers and service worker preload realms). It provides the
// instance API (on/once/off/emit/prependListener/removeAllListeners/
// listeners/rawListeners/listenerCount/eventNames/setMaxListeners/
// getMaxListeners, the 'newListener' and 'removeListener' events and the
// max-listener warning) and `EventEmitter.defaultMaxListeners`; the module
// level helpers of `node:events` are not included.
//
// As in lib/events.js, listener state lives in the instance's own `_events` /
// `_eventsCount` / `_maxListeners` properties, so the methods are generic over
// their receiver and JavaScript classes can extend the constructor.
v8::Local<v8::Function> CreateNodeEventEmitterConstructor(
    v8::Local<v8::Context> context);

}  // namespace gin_helper

#endif  // ELECTRON_SHELL_COMMON_GIN_HELPER_NODE_EVENT_EMITTER_H_
