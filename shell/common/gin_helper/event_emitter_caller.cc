// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/gin_helper/event_emitter_caller.h"

#include "shell/common/node_includes.h"

namespace gin_helper::internal {

v8::Local<v8::Value> CallMethodWithArgs(
    v8::EscapableHandleScope& scope,
    v8::Isolate* isolate,
    v8::Local<v8::Object> obj,
    const std::string_view method,
    const base::span<v8::Local<v8::Value>> args) {
  // CallbackScope and MakeCallback both require an active node::Environment
  if (!node::Environment::GetCurrent(isolate))
    return scope.Escape(v8::False(isolate));

  // |obj| is reused as the async resource; with async_context{0,0} no async
  // hooks fire, so the resource is unobserved.
  node::CallbackScope callback_scope{isolate, obj, node::async_context{0, 0}};

  // Perform microtask checkpoint after running JavaScript.
  v8::MicrotasksScope microtasks_scope(obj->GetCreationContextChecked(isolate),
                                       v8::MicrotasksScope::kRunMicrotasks);

  // node::MakeCallback will also run pending tasks in Node.js.
  v8::MaybeLocal<v8::Value> ret =
      node::MakeCallback(isolate, obj, gin::StringToSymbol(isolate, method),
                         args.size(), args.data(), {0, 0});

  // If the JS function throws an exception (doesn't return a value) the result
  // of MakeCallback will be empty and therefore ToLocal will be false, in this
  // case we need to return "false" as that indicates that the event emitter did
  // not handle the event
  if (v8::Local<v8::Value> localRet; ret.ToLocal(&localRet))
    return scope.Escape(localRet);

  return scope.Escape(v8::False(isolate));
}

}  // namespace gin_helper::internal
