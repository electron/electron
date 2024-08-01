// Copyright (c) 2020 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "base/environment.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/node_includes.h"

namespace {

v8::Local<v8::Value> GetVar(v8::Isolate* isolate, const std::string& name) {
  std::string value;
  if (base::Environment::Create()->GetVar(name, &value)) {
    return gin::StringToV8(isolate, value);
  } else {
    return v8::Null(isolate);
  }
}

bool HasVar(const std::string& name) {
  return base::Environment::Create()->HasVar(name);
}

bool SetVar(const std::string& name, const std::string& value) {
  return base::Environment::Create()->SetVar(name, value);
}

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  gin_helper::Dictionary dict(context->GetIsolate(), exports);
  dict.SetMethod("getVar", &GetVar);
  dict.SetMethod("hasVar", &HasVar);
  dict.SetMethod("setVar", &SetVar);
}

}  // namespace

NODE_LINKED_BINDING_CONTEXT_AWARE(electron_common_environment, Initialize)
