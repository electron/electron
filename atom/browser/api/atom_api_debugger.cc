// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_debugger.h"

#include <string>

#include "atom/common/native_mate_converters/callback.h"
#include "atom/common/native_mate_converters/value_converter.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "content/public/browser/devtools_agent_host.h"
#include "content/public/browser/web_contents.h"
#include "native_mate/dictionary.h"

#include "atom/common/node_includes.h"

using content::DevToolsAgentHost;

namespace atom {

namespace api {

Debugger::Debugger(v8::Isolate* isolate, content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents), web_contents_(web_contents) {
  Init(isolate);
}

Debugger::~Debugger() {}

void Debugger::AgentHostClosed(DevToolsAgentHost* agent_host) {
  DCHECK(agent_host == agent_host_);
  agent_host_ = nullptr;
  ClearPendingRequests();
  Emit("detach", "target closed");
}

void Debugger::DispatchProtocolMessage(DevToolsAgentHost* agent_host,
                                       const std::string& message) {
  DCHECK(agent_host == agent_host_);

  v8::Locker locker(isolate());
  v8::HandleScope handle_scope(isolate());

  std::unique_ptr<base::Value> parsed_message = base::JSONReader::Read(message);
  if (!parsed_message || !parsed_message->is_dict())
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

void Debugger::RenderFrameHostChanged(content::RenderFrameHost* old_rfh,
                                      content::RenderFrameHost* new_rfh) {
  if (agent_host_) {
    agent_host_->DisconnectWebContents();
    auto* web_contents = content::WebContents::FromRenderFrameHost(new_rfh);
    agent_host_->ConnectWebContents(web_contents);
  }
}

void Debugger::Attach(mate::Arguments* args) {
  std::string protocol_version;
  args->GetNext(&protocol_version);

  if (agent_host_) {
    args->ThrowError("Debugger is already attached to the target");
    return;
  }

  if (!protocol_version.empty() &&
      !DevToolsAgentHost::IsSupportedProtocolVersion(protocol_version)) {
    args->ThrowError("Requested protocol version is not supported");
    return;
  }

  agent_host_ = DevToolsAgentHost::GetOrCreateFor(web_contents_);
  if (!agent_host_) {
    args->ThrowError("No target available");
    return;
  }

  agent_host_->AttachClient(this);
}

bool Debugger::IsAttached() {
  return agent_host_ && agent_host_->IsAttached();
}

void Debugger::Detach() {
  if (!agent_host_)
    return;
  agent_host_->DetachClient(this);
  AgentHostClosed(agent_host_.get());
}

void Debugger::SendCommand(mate::Arguments* args) {
  if (!agent_host_)
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
    request.Set("params", base::WrapUnique(command_params.DeepCopy()));

  std::string json_args;
  base::JSONWriter::Write(request, &json_args);
  agent_host_->DispatchProtocolMessage(this, json_args);
}

void Debugger::ClearPendingRequests() {
  if (pending_requests_.empty())
    return;
  base::Value error(base::Value::Type::DICTIONARY);
  base::Value error_msg("target closed while handling command");
  error.SetKey("message", std::move(error_msg));
  for (const auto& it : pending_requests_)
    it.second.Run(error, base::Value());
}

// static
mate::Handle<Debugger> Debugger::Create(v8::Isolate* isolate,
                                        content::WebContents* web_contents) {
  return mate::CreateHandle(isolate, new Debugger(isolate, web_contents));
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

}  // namespace api

}  // namespace atom

namespace {

using atom::api::Debugger;

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  mate::Dictionary(isolate, exports)
      .Set("Debugger", Debugger::GetConstructor(isolate)->GetFunction());
}

}  // namespace

NODE_BUILTIN_MODULE_CONTEXT_AWARE(atom_browser_debugger, Initialize);
