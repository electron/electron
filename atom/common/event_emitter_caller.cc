// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/event_emitter_caller.h"

#include "base/memory/scoped_ptr.h"
#include "third_party/WebKit/public/web/WebScopedMicrotaskSuppression.h"

#include "atom/common/node_includes.h"

namespace mate {

namespace internal {

namespace {

// Returns whether current process is browser process, currently we detect it
// by checking whether current has used V8 Lock, but it might be a bad idea.
inline bool IsBrowserProcess() {
  return v8::Locker::IsActive();
}

}  // namespace

v8::Local<v8::Value> CallEmitWithArgs(v8::Isolate* isolate,
                                      v8::Local<v8::Object> obj,
                                      ValueVector* args) {
  // Perform microtask checkpoint after running JavaScript.
  scoped_ptr<blink::WebScopedRunV8Script> script_scope(
      IsBrowserProcess() ? nullptr : new blink::WebScopedRunV8Script(isolate));
  // Use node::MakeCallback to call the callback, and it will also run pending
  // tasks in Node.js.
  return node::MakeCallback(
      isolate, obj, "emit", args->size(), &args->front());
}

}  // namespace internal

}  // namespace mate
