// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/node_util.h"

#include "base/compiler_specific.h"
#include "base/containers/to_value_list.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/values.h"
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

  if (compiled.IsEmpty()) {
    // TODO(samuelmaddock): how can we get the compilation error message?
    LOG(ERROR) << "CompileAndCall failed to compile electron script (" << id
               << ")";
    return {};
  }

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
    LOG(ERROR) << "CompileAndCall failed to evaluate electron script (" << id
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

node::Environment* CreateEnvironment(v8::Isolate* isolate,
                                     node::IsolateData* isolate_data,
                                     v8::Local<v8::Context> context,
                                     const std::vector<std::string>& args,
                                     const std::vector<std::string>& exec_args,
                                     node::EnvironmentFlags::Flags env_flags,
                                     std::string_view process_type) {
  v8::TryCatch try_catch{isolate};
  node::Environment* env = node::CreateEnvironment(isolate_data, context, args,
                                                   exec_args, env_flags);
  if (auto message = try_catch.Message(); !message.IsEmpty()) {
    base::Value::Dict dict;

    if (std::string str; gin::ConvertFromV8(isolate, message->Get(), &str))
      dict.Set("message", std::move(str));

    if (std::string str; gin::ConvertFromV8(
            isolate, message->GetScriptOrigin().ResourceName(), &str)) {
      const auto line_num = message->GetLineNumber(context).FromJust();
      const auto line_str = base::NumberToString(line_num);
      dict.Set("location", base::StrCat({", at ", str, ":", line_str}));
    }

    if (std::string str; gin::ConvertFromV8(
            isolate, message->GetSourceLine(context).ToLocalChecked(), &str))
      dict.Set("source_line", std::move(str));

    if (!std::empty(process_type))
      dict.Set("process_type", process_type);

    if (auto list = base::ToValueList(args); !std::empty(list))
      dict.Set("args", std::move(list));

    if (auto list = base::ToValueList(exec_args); !std::empty(list))
      dict.Set("exec_args", std::move(list));

    std::string errstr = "Failed to initialize Node.js.";
    if (std::optional<std::string> jsonstr = base::WriteJsonWithOptions(
            dict, base::JsonOptions::OPTIONS_PRETTY_PRINT))
      errstr += base::StrCat({" ", *jsonstr});

    LOG(ERROR) << errstr;
  }

  return env;
}

}  // namespace electron::util
