// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/gin_helper/event_emitter_caller.h"

#include "base/logging.h"
#include "shell/common/gin_helper/locker.h"
#include "shell/common/gin_helper/microtasks_scope.h"
#include "shell/common/node_includes.h"

namespace {

v8::MaybeLocal<v8::Value> MakeCallback(
    v8::Isolate* isolate,
    v8::Local<v8::Object> obj,
    v8::Local<v8::String> method,
    gin_helper::internal::ValueVector* argv) {
  v8::Local<v8::Context> context = isolate->GetCurrentContext();

  v8::Local<v8::Value> function_value;
  if (!obj->Get(context, method).ToLocal(&function_value) ||
      !function_value->IsFunction()) {
    std::string method_name;
    if (!method.IsEmpty() &&
        gin::ConvertFromV8(isolate, method, &method_name)) {
      LOG(ERROR) << "MakeCallback: " << method_name << " is not a function";
    }
    return v8::Boolean::New(isolate, false);
  }

  v8::Local<v8::Function> function = function_value.As<v8::Function>();
  v8::MaybeLocal<v8::Value> func_result;

  v8::TryCatch try_catch(isolate);
  try_catch.SetVerbose(true);  // Display error in console.

  func_result = function->Call(context, obj, argv->size(), argv->data());

  if (try_catch.HasCaught()) {
    // Trigger uncaught error dialog when running Node in the main process.
    if (node::Environment::GetCurrent(context)) {
      node::errors::TriggerUncaughtException(isolate, try_catch);
    }
    return v8::Boolean::New(isolate, false);
  }

  if (func_result.IsEmpty())
    return v8::Boolean::New(isolate, false);

  return func_result;
}

}  // namespace

namespace gin_helper::internal {

v8::Local<v8::Value> CallMethodWithArgs(v8::Isolate* isolate,
                                        v8::Local<v8::Object> obj,
                                        const char* method,
                                        ValueVector* args) {
  // Use creation context to avoid crash when calling from multiple worlds.
  v8::Local<v8::Context> context = obj->GetCreationContextChecked();

  // Perform microtask checkpoint after running JavaScript.
  gin_helper::MicrotasksScope microtasks_scope(isolate, true);
  v8::HandleScope handle_scope(isolate);
  v8::Context::Scope context_scope(context);

  v8::MaybeLocal<v8::Value> ret =
      MakeCallback(isolate, obj, gin::StringToV8(isolate, method), args);
  // If the JS function throws an exception (doesn't return a value) the result
  // of MakeCallback will be empty and therefore ToLocal will be false, in this
  // case we need to return "false" as that indicates that the event emitter did
  // not handle the event
  v8::Local<v8::Value> localRet;
  if (ret.ToLocal(&localRet)) {
    return localRet;
  }
  return v8::Boolean::New(isolate, false);
}

}  // namespace gin_helper::internal
