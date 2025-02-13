// Copyright (c) 2019 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "electron/shell/renderer/electron_ipc_native.h"

#include "base/trace_event/trace_event.h"
#include "shell/common/gin_converters/blink_converter.h"
#include "shell/common/gin_converters/value_converter.h"
#include "shell/common/node_includes.h"
#include "shell/common/v8_util.h"
#include "third_party/blink/public/web/blink.h"
#include "third_party/blink/public/web/web_message_port_converter.h"

namespace electron::ipc_native {

namespace {

constexpr std::string_view kIpcKey = "ipcNative";

// Gets the private object under kIpcKey
v8::Local<v8::Object> GetIpcObject(const v8::Local<v8::Context>& context) {
  auto* isolate = context->GetIsolate();
  auto binding_key = gin::StringToV8(isolate, kIpcKey);
  auto private_binding_key = v8::Private::ForApi(isolate, binding_key);
  auto global_object = context->Global();
  auto value =
      global_object->GetPrivate(context, private_binding_key).ToLocalChecked();
  if (value.IsEmpty() || !value->IsObject()) {
    LOG(ERROR) << "Attempted to get the 'ipcNative' object but it was missing";
    return {};
  }
  return value->ToObject(context).ToLocalChecked();
}

void InvokeIpcCallback(const v8::Local<v8::Context>& context,
                       const std::string& callback_name,
                       std::vector<v8::Local<v8::Value>> args) {
  TRACE_EVENT0("devtools.timeline", "FunctionCall");
  auto* isolate = context->GetIsolate();

  auto ipcNative = GetIpcObject(context);
  if (ipcNative.IsEmpty())
    return;

  // Only set up the node::CallbackScope if there's a node environment.
  // Sandboxed renderers don't have a node environment.
  std::unique_ptr<node::CallbackScope> callback_scope;
  if (node::Environment::GetCurrent(context)) {
    callback_scope = std::make_unique<node::CallbackScope>(
        isolate, ipcNative, node::async_context{0, 0});
  }

  auto callback_key = gin::ConvertToV8(isolate, callback_name)
                          ->ToString(context)
                          .ToLocalChecked();
  auto callback_value = ipcNative->Get(context, callback_key).ToLocalChecked();
  DCHECK(callback_value->IsFunction());  // set by init.ts
  auto callback = callback_value.As<v8::Function>();
  std::ignore = callback->Call(context, ipcNative, args.size(), args.data());
}

}  // namespace

void EmitIPCEvent(const v8::Local<v8::Context>& context,
                  bool internal,
                  const std::string& channel,
                  std::vector<v8::Local<v8::Value>> ports,
                  v8::Local<v8::Value> args) {
  auto* isolate = context->GetIsolate();

  v8::HandleScope handle_scope(isolate);
  v8::Context::Scope context_scope(context);
  v8::MicrotasksScope script_scope(isolate, context->GetMicrotaskQueue(),
                                   v8::MicrotasksScope::kRunMicrotasks);

  std::vector<v8::Local<v8::Value>> argv = {
      gin::ConvertToV8(isolate, internal), gin::ConvertToV8(isolate, channel),
      gin::ConvertToV8(isolate, ports), args};

  InvokeIpcCallback(context, "onMessage", argv);
}

}  // namespace electron::ipc_native
