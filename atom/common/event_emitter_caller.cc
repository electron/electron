// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/event_emitter_caller.h"

namespace mate {

namespace internal {

v8::Local<v8::Value> CallEmitWithArgs(v8::Isolate* isolate,
                                      v8::Local<v8::Object> obj,
                                      ValueVector* args) {
  v8::Local<v8::String> emit_name = StringToSymbol(isolate, "emit");
  v8::Local<v8::Value> emit = obj->Get(emit_name);
  if (emit.IsEmpty() || !emit->IsFunction()) {
    isolate->ThrowException(v8::Exception::TypeError(
        StringToV8(isolate, "\"emit\" is not a function")));
    return v8::Undefined(isolate);
  }

  v8::MaybeLocal<v8::Value> result = emit.As<v8::Function>()->Call(
      isolate->GetCurrentContext(), obj, args->size(), &args->front());
  if (result.IsEmpty()) {
    return v8::Undefined(isolate);
  }

  return result.ToLocalChecked();
}

}  // namespace internal

}  // namespace mate
