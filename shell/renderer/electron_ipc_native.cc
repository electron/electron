// Copyright (c) 2019 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "electron/shell/renderer/electron_ipc_native.h"

#include <optional>

#include "base/logging.h"
#include "base/trace_event/trace_event.h"
#include "shell/common/gin_converters/blink_converter.h"
#include "shell/common/gin_converters/value_converter.h"
#include "shell/common/gin_helper/node_event_emitter.h"
#include "shell/common/node_includes.h"
#include "shell/common/v8_util.h"

namespace electron::ipc_native {

namespace {

constexpr std::string_view kIpcKey = "ipcNative";

// The hidden `{ ipcRenderer, ipcRendererInternal }` object registered by
// lib/renderer/ipc-native-setup.ts.
v8::Local<v8::Object> GetIpcObject(v8::Isolate* const isolate,
                                   const v8::Local<v8::Context>& context) {
  auto binding_key = gin::StringToV8(isolate, kIpcKey);
  auto private_binding_key = v8::Private::ForApi(isolate, binding_key);
  auto global_object = context->Global();
  auto value =
      global_object->GetPrivate(context, private_binding_key).ToLocalChecked();
  // Nothing in this context listens for IPC (no preload script ran here).
  if (value.IsEmpty() || !value->IsObject())
    return {};
  return value.As<v8::Object>();
}

}  // namespace

void EmitIPCEvent(v8::Isolate* const isolate,
                  const v8::Local<v8::Context>& context,
                  bool internal,
                  const std::string& channel,
                  std::vector<v8::Local<v8::Value>> ports,
                  v8::Local<v8::Value> args) {
  TRACE_EVENT0("devtools.timeline", "FunctionCall");
  v8::HandleScope handle_scope(isolate);
  v8::Context::Scope context_scope(context);
  v8::MicrotasksScope script_scope(isolate, context->GetMicrotaskQueue(),
                                   v8::MicrotasksScope::kRunMicrotasks);

  v8::Local<v8::Object> ipc_native = GetIpcObject(isolate, context);
  if (ipc_native.IsEmpty())
    return;
  v8::Local<v8::Value> emitter_value;
  if (!ipc_native
           ->Get(context,
                 gin::StringToSymbol(
                     isolate, internal ? "ipcRendererInternal" : "ipcRenderer"))
           .ToLocal(&emitter_value) ||
      !emitter_value->IsObject()) {
    return;
  }
  v8::Local<v8::Object> emitter = emitter_value.As<v8::Object>();

  // Only set up the node::CallbackScope if there's a node environment.
  // Sandboxed renderers don't have a node environment.
  std::optional<node::CallbackScope> callback_scope;
  if (auto* env = node::Environment::GetCurrent(context)) {
    callback_scope.emplace(env, emitter, node::async_context{0, 0});
  }

  // emitter.emit(channel, { sender, ports }, ...args)
  v8::Local<v8::Object> event = v8::Object::New(isolate);
  event
      ->CreateDataProperty(context, gin::StringToSymbol(isolate, "sender"),
                           emitter)
      .Check();
  event
      ->CreateDataProperty(context, gin::StringToSymbol(isolate, "ports"),
                           gin::ConvertToV8(isolate, ports))
      .Check();
  v8::LocalVector<v8::Value> argv(isolate, {event});
  if (args->IsArray()) {
    v8::Local<v8::Array> list = args.As<v8::Array>();
    argv.reserve(list->Length() + 1);
    for (uint32_t i = 0; i < list->Length(); ++i) {
      v8::Local<v8::Value> arg;
      if (!list->Get(context, i).ToLocal(&arg))
        return;
      argv.push_back(arg);
    }
  }

  v8::TryCatch try_catch(isolate);
  try_catch.SetVerbose(true);  // report listener exceptions to the console
  gin_helper::EmitEvent(isolate, emitter, gin::StringToV8(isolate, channel),
                        argv);
}

}  // namespace electron::ipc_native
