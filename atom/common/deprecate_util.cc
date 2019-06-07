// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <string>

#include "atom/common/deprecate_util.h"

namespace atom {

v8::Maybe<bool> EmitDeprecationWarning(node::Environment* env,
                                       const char* warning,
                                       const char* type = nullptr,
                                       const char* code = nullptr) {
  v8::HandleScope handle_scope(env->isolate());
  v8::Context::Scope context_scope(env->context());

  v8::Local<v8::Object> process = env->process_object();
  v8::Local<v8::Value> emit_warning;
  if (!process->Get(env->context(), env->emit_warning_string())
           .ToLocal(&emit_warning)) {
    return v8::Nothing<bool>();
  }

  if (!emit_warning->IsFunction())
    return v8::Just(false);

  int argc = 0;
  v8::Local<v8::Value> args[3];  // warning, type, code

  if (!v8::String::NewFromUtf8(env->isolate(), warning,
                               v8::NewStringType::kNormal)
           .ToLocal(&args[argc++])) {
    return v8::Nothing<bool>();
  }
  if (type != nullptr) {
    if (!v8::String::NewFromOneByte(env->isolate(),
                                    reinterpret_cast<const uint8_t*>(type),
                                    v8::NewStringType::kNormal)
             .ToLocal(&args[argc++])) {
      return v8::Nothing<bool>();
    }
    if (code != nullptr &&
        !v8::String::NewFromOneByte(env->isolate(),
                                    reinterpret_cast<const uint8_t*>(code),
                                    v8::NewStringType::kNormal)
             .ToLocal(&args[argc++])) {
      return v8::Nothing<bool>();
    }
  }

  // MakeCallback() unneeded because emitWarning is internal code, it calls
  // process.emit('warning', ...), but does so on the nextTick.
  if (emit_warning.As<v8::Function>()
          ->Call(env->context(), process, argc, args)
          .IsEmpty()) {
    return v8::Nothing<bool>();
  }
  return v8::Just(true);
}

}  // namespace atom
