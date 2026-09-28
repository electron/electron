// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_GIN_HELPER_NODE_EVENT_EMITTER_H_
#define ELECTRON_SHELL_COMMON_GIN_HELPER_NODE_EVENT_EMITTER_H_

#include "base/containers/span.h"
#include "v8/include/v8-forward.h"
#include "v8/include/v8-local-handle.h"

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

// The shared EventEmitter class for `context`, created on first use. Electron's
// own emitters (ipcRenderer, the sandboxed preload's process object) and the
// class handed to scripts through the `electron_common_events` binding are all
// this one, so they share a prototype.
v8::Local<v8::Function> GetNodeEventEmitterConstructor(
    v8::Local<v8::Context> context);

// `new EventEmitter()` of the shared class.
v8::Local<v8::Object> NewNodeEventEmitter(v8::Local<v8::Context> context);

// `emitter.emit(type, ...args)`. When `emitter` is an instance of the class
// above the listeners are invoked directly from C++; for any other object
// (a Node.js EventEmitter in a renderer with Node.js integration) its `emit`
// method is called. Returns false if an exception was thrown; it is left
// pending for the caller's v8::TryCatch.
bool EmitEvent(v8::Isolate* isolate,
               v8::Local<v8::Object> emitter,
               v8::Local<v8::Value> type,
               base::span<v8::Local<v8::Value>> args);

}  // namespace gin_helper

#endif  // ELECTRON_SHELL_COMMON_GIN_HELPER_NODE_EVENT_EMITTER_H_
