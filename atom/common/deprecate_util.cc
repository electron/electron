// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/deprecate_util.h"

namespace atom {

// msg/type/code arguments match Node's process.emitWarning() method. See
// https://nodejs.org/api/process.html#process_process_emitwarning_warning_type_code_ctor
v8::Maybe<bool> EmitDeprecationWarning(node::Environment* env,
                                       std::string warning_msg,
                                       std::string warning_type = "",
                                       std::string warning_code = "") {
  v8::HandleScope handle_scope(env->isolate());
  v8::Context::Scope context_scope(env->context());

  const char* warning = warning_msg.empty() ? nullptr : warning_msg.c_str();
  const char* type = warning_type.empty() ? nullptr : warning_type.c_str();
  const char* code = warning_code.empty() ? nullptr : warning_code.c_str();

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
    if (!v8::String::NewFromUtf8(env->isolate(), type,
                                 v8::NewStringType::kNormal)
             .ToLocal(&args[argc++])) {
      return v8::Nothing<bool>();
    }
    if (code != nullptr && !v8::String::NewFromUtf8(env->isolate(), code,
                                                    v8::NewStringType::kNormal)
                                .ToLocal(&args[argc++])) {
      return v8::Nothing<bool>();
    }
  }

  if (emit_warning.As<v8::Function>()
          ->Call(env->context(), process, argc, args)
          .IsEmpty()) {
    return v8::Nothing<bool>();
  }
  return v8::Just(true);
}

}  // namespace atom
