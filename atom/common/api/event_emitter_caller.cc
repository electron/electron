// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/api/event_emitter_caller.h"

#include "atom/common/api/locker.h"
#include "atom/common/node_includes.h"

namespace mate {

namespace internal {

v8::Local<v8::Value> CallEmitWithArgs(v8::Isolate* isolate,
                                      v8::Local<v8::Object> obj,
                                      ValueVector* args) {
  // Perform microtask checkpoint after running JavaScript.
  v8::MicrotasksScope script_scope(
      isolate, v8::MicrotasksScope::kRunMicrotasks);
  // Use node::MakeCallback to call the callback, and it will also run pending
  // tasks in Node.js.
  return node::MakeCallback(
      isolate, obj, "emit", args->size(), &args->front());
}

}  // namespace internal

}  // namespace mate
