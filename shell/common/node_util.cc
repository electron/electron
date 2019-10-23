// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/node_util.h"
#include "shell/common/node_includes.h"
#include "third_party/electron_node/src/node_native_module_env.h"

namespace electron {

namespace util {

v8::MaybeLocal<v8::Value> CompileAndCall(
    v8::Local<v8::Context> context,
    const char* id,
    std::vector<v8::Local<v8::String>>* parameters,
    std::vector<v8::Local<v8::Value>>* arguments,
    node::Environment* optional_env) {
  v8::Isolate* isolate = context->GetIsolate();
  v8::MaybeLocal<v8::Function> compiled =
      node::native_module::NativeModuleEnv::LookupAndCompile(
          context, id, parameters, optional_env);
  if (compiled.IsEmpty()) {
    return v8::MaybeLocal<v8::Value>();
  }
  v8::Local<v8::Function> fn = compiled.ToLocalChecked().As<v8::Function>();
  return fn->Call(context, v8::Null(isolate), arguments->size(),
                  arguments->data());
}

}  // namespace util

}  // namespace electron
