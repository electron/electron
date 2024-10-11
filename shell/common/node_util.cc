// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/node_util.h"

#include "base/compiler_specific.h"
#include "base/logging.h"
#include "gin/converter.h"
#include "gin/dictionary.h"
#include "shell/browser/javascript_environment.h"
#include "shell/common/gin_converters/callback_converter.h"
#include "shell/common/node_includes.h"

namespace electron::util {

v8::MaybeLocal<v8::Value> CompileAndCall(
    v8::Local<v8::Context> context,
    const char* id,
    std::vector<v8::Local<v8::String>>* parameters,
    std::vector<v8::Local<v8::Value>>* arguments) {
  v8::Isolate* isolate = context->GetIsolate();
  v8::TryCatch try_catch(isolate);

  thread_local node::builtins::BuiltinLoader builtin_loader;
  v8::MaybeLocal<v8::Function> compiled = builtin_loader.LookupAndCompile(
      context, id, parameters, node::Realm::GetCurrent(context));

  if (compiled.IsEmpty())
    return v8::MaybeLocal<v8::Value>();

  v8::Local<v8::Function> fn = compiled.ToLocalChecked().As<v8::Function>();
  v8::MaybeLocal<v8::Value> ret = fn->Call(
      context, v8::Null(isolate), arguments->size(), arguments->data());

  // This will only be caught when something has gone terrible wrong as all
  // electron scripts are wrapped in a try {} catch {} by webpack
  if (try_catch.HasCaught()) {
    std::string msg = "no error message";
    if (!try_catch.Message().IsEmpty()) {
      gin::ConvertFromV8(isolate, try_catch.Message()->Get(), &msg);
    } else if (try_catch.HasTerminated()) {
      msg = "script execution has been terminated";
    }
    LOG(ERROR) << "Failed to CompileAndCall electron script (" << id
               << "): " << msg;
  }
  return ret;
}

void EmitWarning(const std::string_view warning_msg,
                 const std::string_view warning_type) {
  EmitWarning(JavascriptEnvironment::GetIsolate(), warning_msg, warning_type);
}

void EmitWarning(v8::Isolate* isolate,
                 const std::string_view warning_msg,
                 const std::string_view warning_type) {
  v8::HandleScope scope{isolate};
  gin::Dictionary process{
      isolate, node::Environment::GetCurrent(isolate)->process_object()};
  base::RepeatingCallback<void(std::string_view, std::string_view,
                               std::string_view)>
      emit_warning;
  process.Get("emitWarning", &emit_warning);
  emit_warning.Run(warning_msg, warning_type, "");
}

// SAFETY: There is no node::Buffer API that passes the UNSAFE_BUFFER_USAGE
// test, so let's isolate the unsafe API use into this function. Instead of
// calling `Buffer::Data()` and `Buffer::Length()` directly, the rest of our
// code should prefer to use spans returned by this function.
base::span<uint8_t> as_byte_span(v8::Local<v8::Value> node_buffer) {
  auto* data = reinterpret_cast<uint8_t*>(node::Buffer::Data(node_buffer));
  const auto size = node::Buffer::Length(node_buffer);
  return UNSAFE_BUFFERS(base::span{data, size});
}

}  // namespace electron::util
