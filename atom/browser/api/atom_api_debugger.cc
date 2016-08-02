// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_debugger.h"

#include <string>

#include "atom/browser/atom_browser_main_parts.h"
#include "atom/common/native_mate_converters/callback.h"
#include "atom/common/native_mate_converters/value_converter.h"
#include "atom/common/node_includes.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "content/public/browser/devtools_agent_host.h"
#include "content/public/browser/web_contents.h"
#include "native_mate/dictionary.h"
#include "native_mate/object_template_builder.h"

using content::DevToolsAgentHost;

namespace atom {

namespace api {

namespace {

// The wrapDebugger funtion which is implemented in JavaScript.
using WrapDebuggerCallback = base::Callback<void(v8::Local<v8::Value>)>;
WrapDebuggerCallback g_wrap_debugger;

}  // namespace

Debugger::Debugger(v8::Isolate* isolate, content::WebContents* web_contents)
    : web_contents_(web_contents),
      previous_request_id_(0) {
  Init(isolate);
}

Debugger::~Debugger() {
}

void Debugger::AgentHostClosed(DevToolsAgentHost* agent_host,
                               bool replaced_with_another_client) {
  std::string detach_reason = "target closed";
  if (replaced_with_another_client)
    detach_reason = "replaced with devtools";
  Emit("detach", detach_reason);
}

void Debugger::DispatchProtocolMessage(DevToolsAgentHost* agent_host,
                                       const std::string& message) {
  DCHECK(agent_host == agent_host_.get());

  std::unique_ptr<base::Value> parsed_message(base::JSONReader::Read(message));
  if (!parsed_message->IsType(base::Value::TYPE_DICTIONARY))
    return;

  base::DictionaryValue* dict =
      static_cast<base::DictionaryValue*>(parsed_message.get());
  int id;
  if (!dict->GetInteger("id", &id)) {
    std::string method;
    if (!dict->GetString("method", &method))
      return;
    base::DictionaryValue* params_value = nullptr;
    base::DictionaryValue params;
    if (dict->GetDictionary("params", &params_value))
      params.Swap(params_value);
    Emit("message", method, params);
  } else {
    auto send_command_callback = pending_requests_[id];
    pending_requests_.erase(id);
    if (send_command_callback.is_null())
      return;
    base::DictionaryValue* error_body = nullptr;
    base::DictionaryValue error;
    if (dict->GetDictionary("error", &error_body))
      error.Swap(error_body);

    base::DictionaryValue* result_body = nullptr;
    base::DictionaryValue result;
    if (dict->GetDictionary("result", &result_body))
      result.Swap(result_body);
    send_command_callback.Run(error, result);
  }
}

void Debugger::Attach(mate::Arguments* args) {
  std::string protocol_version;
  args->GetNext(&protocol_version);

  if (!protocol_version.empty() &&
      !DevToolsAgentHost::IsSupportedProtocolVersion(protocol_version)) {
    args->ThrowError("Requested protocol version is not supported");
    return;
  }
  agent_host_ = DevToolsAgentHost::GetOrCreateFor(web_contents_);
  if (!agent_host_.get()) {
    args->ThrowError("No target available");
    return;
  }
  if (agent_host_->IsAttached()) {
    args->ThrowError("Another debugger is already attached to this target");
    return;
  }

  agent_host_->AttachClient(this);
}

bool Debugger::IsAttached() {
  return agent_host_.get() ? agent_host_->IsAttached() : false;
}

void Debugger::Detach() {
  if (!agent_host_.get())
    return;
  agent_host_->DetachClient();
  AgentHostClosed(agent_host_.get(), false);
  agent_host_ = nullptr;
}

void Debugger::SendCommand(mate::Arguments* args) {
  if (!agent_host_.get())
    return;

  std::string method;
  if (!args->GetNext(&method)) {
    args->ThrowError();
    return;
  }
  base::DictionaryValue command_params;
  args->GetNext(&command_params);
  SendCommandCallback callback;
  args->GetNext(&callback);

  base::DictionaryValue request;
  int request_id = ++previous_request_id_;
  pending_requests_[request_id] = callback;
  request.SetInteger("id", request_id);
  request.SetString("method", method);
  if (!command_params.empty())
    request.Set("params", command_params.DeepCopy());

  std::string json_args;
  base::JSONWriter::Write(request, &json_args);
  agent_host_->DispatchProtocolMessage(json_args);
}

// static
mate::Handle<Debugger> Debugger::Create(
    v8::Isolate* isolate,
    content::WebContents* web_contents) {
  auto handle = mate::CreateHandle(
      isolate, new Debugger(isolate, web_contents));
  g_wrap_debugger.Run(handle.ToV8());
  return handle;
}

// static
void Debugger::BuildPrototype(v8::Isolate* isolate,
                              v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(mate::StringToV8(isolate, "Debugger"));
  mate::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
      .SetMethod("attach", &Debugger::Attach)
      .SetMethod("isAttached", &Debugger::IsAttached)
      .SetMethod("detach", &Debugger::Detach)
      .SetMethod("sendCommand", &Debugger::SendCommand);
}

void SetWrapDebugger(const WrapDebuggerCallback& callback) {
  g_wrap_debugger = callback;
}

}  // namespace api

}  // namespace atom

namespace {

void Initialize(v8::Local<v8::Object> exports, v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context, void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  mate::Dictionary dict(isolate, exports);
  dict.SetMethod("_setWrapDebugger", &atom::api::SetWrapDebugger);
}

}  // namespace

NODE_MODULE_CONTEXT_AWARE_BUILTIN(atom_browser_debugger, Initialize);
