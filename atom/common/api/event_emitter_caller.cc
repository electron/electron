// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/api/event_emitter_caller.h"

#include "atom/common/api/locker.h"
#include "atom/common/node_includes.h"

namespace mate {

namespace internal {

v8::Local<v8::Value> CallMethodWithArgs(v8::Isolate* isolate,
                                        v8::Local<v8::Object> obj,
                                        const char* method,
                                        ValueVector* args) {
  // Perform microtask checkpoint after running JavaScript.
  v8::MicrotasksScope script_scope(isolate,
                                   v8::MicrotasksScope::kRunMicrotasks);
  // Use node::MakeCallback to call the callback, and it will also run pending
  // tasks in Node.js.
  v8::MaybeLocal<v8::Value> ret = node::MakeCallback(isolate, obj, method, args->size(), &args->front(),
                            {0, 0});
  if (ret.IsEmpty()) {
    return v8::Boolean::New(isolate, false);
  }
  return ret.ToLocalChecked();
}

}  // namespace internal

}  // namespace mate
