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

// What a receiver's listener table holds for `type` after a change made by
// one of the methods InstallListenerMethods() defines.
enum class ListenerChange {
  kObserved,       // `type` has at least one listener
  kUnobserved,     // `type` has no listeners left
  kUnobservedAll,  // no event has; `type` is undefined
};
using ListenerChangeCallback = void (*)(v8::Isolate* isolate,
                                        v8::Local<v8::Object> emitter,
                                        v8::Local<v8::Value> type,
                                        ListenerChange change);

// Defines native addListener / on / prependListener / removeListener / off /
// removeAllListeners on `prototype`: the class above's own implementations,
// which are generic over their receiver, so `prototype` can sit in front of
// Node.js's EventEmitter.prototype and leave once(), emit() and the rest to
// it. Each change one of them makes to a receiver's listener table is
// reported to `callback` after the fact.
void InstallListenerMethods(v8::Local<v8::Context> context,
                            v8::Local<v8::Object> prototype,
                            ListenerChangeCallback callback);

}  // namespace gin_helper

#endif  // ELECTRON_SHELL_COMMON_GIN_HELPER_NODE_EVENT_EMITTER_H_
