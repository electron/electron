// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/node_util.h"

#include "base/logging.h"
#include "shell/common/node_includes.h"

namespace electron::util {

v8::MaybeLocal<v8::Value> CompileAndCall(
    v8::Local<v8::Context> context,
    const char* id,
    std::vector<v8::Local<v8::String>>* parameters,
    std::vector<v8::Local<v8::Value>>* arguments,
    node::Environment* optional_env) {
  v8::Isolate* isolate = context->GetIsolate();
  v8::TryCatch try_catch(isolate);
  v8::MaybeLocal<v8::Function> compiled =
      node::native_module::NativeModuleEnv::LookupAndCompile(
          context, id, parameters, optional_env);
  if (compiled.IsEmpty()) {
    return v8::MaybeLocal<v8::Value>();
  }
  v8::Local<v8::Function> fn = compiled.ToLocalChecked().As<v8::Function>();
  v8::MaybeLocal<v8::Value> ret = fn->Call(
      context, v8::Null(isolate), arguments->size(), arguments->data());
  // This will only be caught when something has gone terrible wrong as all
  // electron scripts are wrapped in a try {} catch {} by webpack
  if (try_catch.HasCaught()) {
    LOG(ERROR) << "Failed to CompileAndCall electron script: " << id;
  }
  return ret;
}

}  // namespace electron::util
