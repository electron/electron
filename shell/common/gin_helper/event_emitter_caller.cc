// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/gin_helper/event_emitter_caller.h"

#include "shell/common/gin_helper/microtasks_scope.h"
#include "shell/common/node_includes.h"

namespace gin_helper::internal {

v8::Local<v8::Value> CallMethodWithArgs(
    v8::Isolate* isolate,
    v8::Local<v8::Object> obj,
    const char* method,
    const base::span<v8::Local<v8::Value>> args) {
  v8::EscapableHandleScope handle_scope{isolate};

  // CallbackScope and MakeCallback both require an active node::Environment
  if (!node::Environment::GetCurrent(isolate))
    return handle_scope.Escape(v8::Boolean::New(isolate, false));

  node::CallbackScope callback_scope{isolate, v8::Object::New(isolate),
                                     node::async_context{0, 0}};

  // Perform microtask checkpoint after running JavaScript.
  gin_helper::MicrotasksScope microtasks_scope{
      isolate, obj->GetCreationContextChecked()->GetMicrotaskQueue(), true,
      v8::MicrotasksScope::kRunMicrotasks};

  // node::MakeCallback will also run pending tasks in Node.js.
  v8::MaybeLocal<v8::Value> ret = node::MakeCallback(
      isolate, obj, method, args.size(), args.data(), {0, 0});

  // If the JS function throws an exception (doesn't return a value) the result
  // of MakeCallback will be empty and therefore ToLocal will be false, in this
  // case we need to return "false" as that indicates that the event emitter did
  // not handle the event
  if (v8::Local<v8::Value> localRet; ret.ToLocal(&localRet))
    return handle_scope.Escape(localRet);

  return handle_scope.Escape(v8::Boolean::New(isolate, false));
}

}  // namespace gin_helper::internal
