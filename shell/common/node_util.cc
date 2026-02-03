// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/node_util.h"

#include "base/compiler_specific.h"
#include "base/containers/to_value_list.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/no_destructor.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/threading/thread_local.h"
#include "base/values.h"
#include "gin/converter.h"
#include "shell/browser/javascript_environment.h"
#include "shell/common/gin_converters/callback_converter.h"
#include "shell/common/node_includes.h"
#include "shell/common/process_util.h"
#include "third_party/electron_node/src/node_process-inl.h"

namespace electron::util {

v8::MaybeLocal<v8::Value> CompileAndCall(
    v8::Isolate* const isolate,
    v8::Local<v8::Context> context,
    const char* id,
    v8::LocalVector<v8::String>* parameters,
    v8::LocalVector<v8::Value>* arguments) {
  v8::TryCatch try_catch{isolate};

  static base::NoDestructor<
      base::ThreadLocalOwnedPointer<node::builtins::BuiltinLoader>>
      builtin_loader;
  if (!builtin_loader->Get()) {
    builtin_loader->Set(base::WrapUnique(new node::builtins::BuiltinLoader));
  }
  v8::MaybeLocal<v8::Function> compiled =
      builtin_loader->Get()->LookupAndCompile(context, id, parameters,
                                              node::Realm::GetCurrent(context));

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
  node::Environment* env = node::Environment::GetCurrent(isolate);
  if (!env) {
    // No Node.js environment available, fall back to console logging.
    LOG(WARNING) << "[" << warning_type << "] " << warning_msg;
    return;
  }
  node::ProcessEmitWarningGeneric(env, warning_msg, warning_type);
}

void EmitDeprecationWarning(const std::string_view warning_msg,
                            const std::string_view deprecation_code) {
  EmitDeprecationWarning(JavascriptEnvironment::GetIsolate(), warning_msg,
                         deprecation_code);
}

void EmitDeprecationWarning(v8::Isolate* isolate,
                            const std::string_view warning_msg,
                            const std::string_view deprecation_code) {
  node::Environment* env = node::Environment::GetCurrent(isolate);
  if (!env) {
    // No Node.js environment available, fall back to console logging.
    LOG(WARNING) << "[DeprecationWarning] " << warning_msg
                 << " (code: " << deprecation_code << ")";
    return;
  }
  node::ProcessEmitWarningGeneric(env, warning_msg, "DeprecationWarning",
                                  deprecation_code);
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
    base::DictValue dict;

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

ExplicitMicrotasksScope::ExplicitMicrotasksScope(v8::MicrotaskQueue* queue)
    : microtask_queue_(queue), original_policy_(queue->microtasks_policy()) {
  // In browser-like processes, some nested run loops (macOS usually) may
  // re-enter. This is safe because we expect the policy was explicit in the
  // first place for those processes. However, in renderer processes, there may
  // be unexpected behavior if this code is triggered within a pending microtask
  // scope.
  if (electron::IsBrowserProcess() || electron::IsUtilityProcess()) {
    DCHECK_EQ(original_policy_, v8::MicrotasksPolicy::kExplicit);
  } else {
    DCHECK_EQ(microtask_queue_->GetMicrotasksScopeDepth(), 0);
  }

  microtask_queue_->set_microtasks_policy(v8::MicrotasksPolicy::kExplicit);
}

ExplicitMicrotasksScope::~ExplicitMicrotasksScope() {
  microtask_queue_->set_microtasks_policy(original_policy_);
}

}  // namespace electron::util

namespace electron::Buffer {

// SAFETY: There is no node::Buffer API that passes the UNSAFE_BUFFER_USAGE
// test, so let's isolate the unsafe API use into this function. Instead of
// calling `Buffer::Data()` and `Buffer::Length()` directly, the rest of our
// code should prefer to use spans returned by this function.
base::span<uint8_t> as_byte_span(v8::Local<v8::Value> node_buffer) {
  auto* data = reinterpret_cast<uint8_t*>(node::Buffer::Data(node_buffer));
  const auto size = node::Buffer::Length(node_buffer);
  return UNSAFE_BUFFERS(base::span{data, size});
}

v8::MaybeLocal<v8::Object> Copy(v8::Isolate* isolate,
                                const base::span<const char> data) {
  // SAFETY: span-friendly version of node::Buffer::Copy()
  return UNSAFE_BUFFERS(node::Buffer::Copy(isolate, data.data(), data.size()));
}

v8::MaybeLocal<v8::Object> Copy(v8::Isolate* isolate,
                                const base::span<const uint8_t> data) {
  return Copy(isolate, base::as_chars(data));
}

}  // namespace electron::Buffer
