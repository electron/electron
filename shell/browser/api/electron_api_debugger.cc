// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/electron_api_debugger.h"

#include <memory>
#include <string>
#include <utility>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "content/public/browser/devtools_agent_host.h"
#include "content/public/browser/web_contents.h"
#include "gin/object_template_builder.h"
#include "gin/per_isolate_data.h"
#include "shell/browser/javascript_environment.h"
#include "shell/common/gin_converters/value_converter.h"
#include "shell/common/node_includes.h"

using content::DevToolsAgentHost;

namespace electron {

namespace api {

gin::WrapperInfo Debugger::kWrapperInfo = {gin::kEmbedderNativeGin};

Debugger::Debugger(v8::Isolate* isolate, content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents), web_contents_(web_contents) {}

Debugger::~Debugger() = default;

void Debugger::AgentHostClosed(DevToolsAgentHost* agent_host) {
  DCHECK(agent_host == agent_host_);
  agent_host_ = nullptr;
  ClearPendingRequests();
  Emit("detach", "target closed");
}

void Debugger::DispatchProtocolMessage(DevToolsAgentHost* agent_host,
                                       base::span<const uint8_t> message) {
  DCHECK(agent_host == agent_host_);

  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();

  v8::Locker locker(isolate);
  v8::HandleScope handle_scope(isolate);

  base::StringPiece message_str(reinterpret_cast<const char*>(message.data()),
                                message.size());
  std::unique_ptr<base::Value> parsed_message =
      base::JSONReader::ReadDeprecated(message_str,
                                       base::JSON_REPLACE_INVALID_CHARACTERS);
  if (!parsed_message || !parsed_message->is_dict())
    return;
  auto* dict = static_cast<base::DictionaryValue*>(parsed_message.get());
  int id;
  if (!dict->GetInteger("id", &id)) {
    std::string method;
    if (!dict->GetString("method", &method))
      return;
    std::string session_id;
    dict->GetString("sessionId", &session_id);
    base::DictionaryValue* params_value = nullptr;
    base::DictionaryValue params;
    if (dict->GetDictionary("params", &params_value))
      params.Swap(params_value);
    Emit("message", method, params, session_id);
  } else {
    auto it = pending_requests_.find(id);
    if (it == pending_requests_.end())
      return;

    gin_helper::Promise<base::DictionaryValue> promise = std::move(it->second);
    pending_requests_.erase(it);

    base::DictionaryValue* error = nullptr;
    if (dict->GetDictionary("error", &error)) {
      std::string message;
      error->GetString("message", &message);
      promise.RejectWithErrorMessage(message);
    } else {
      base::DictionaryValue* result_body = nullptr;
      base::DictionaryValue result;
      if (dict->GetDictionary("result", &result_body)) {
        result.Swap(result_body);
      }
      promise.Resolve(result);
    }
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

void Debugger::Attach(gin::Arguments* args) {
  std::string protocol_version;
  args->GetNext(&protocol_version);

  if (agent_host_) {
    args->ThrowTypeError("Debugger is already attached to the target");
    return;
  }

  if (!protocol_version.empty() &&
      !DevToolsAgentHost::IsSupportedProtocolVersion(protocol_version)) {
    args->ThrowTypeError("Requested protocol version is not supported");
    return;
  }

  agent_host_ = DevToolsAgentHost::GetOrCreateFor(web_contents_);
  if (!agent_host_) {
    args->ThrowTypeError("No target available");
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

v8::Local<v8::Promise> Debugger::SendCommand(gin::Arguments* args) {
  v8::Isolate* isolate = args->isolate();
  gin_helper::Promise<base::DictionaryValue> promise(isolate);
  v8::Local<v8::Promise> handle = promise.GetHandle();

  if (!agent_host_) {
    promise.RejectWithErrorMessage("No target available");
    return handle;
  }

  std::string method;
  if (!args->GetNext(&method)) {
    promise.RejectWithErrorMessage("Invalid method");
    return handle;
  }

  base::DictionaryValue command_params;
  args->GetNext(&command_params);

  std::string session_id;
  if (args->GetNext(&session_id) && session_id.empty()) {
    promise.RejectWithErrorMessage("Empty session id is not allowed");
    return handle;
  }

  base::DictionaryValue request;
  int request_id = ++previous_request_id_;
  pending_requests_.emplace(request_id, std::move(promise));
  request.SetInteger("id", request_id);
  request.SetString("method", method);
  if (!command_params.DictEmpty()) {
    request.Set("params",
                base::Value::ToUniquePtrValue(command_params.Clone()));
  }

  if (!session_id.empty()) {
    request.SetString("sessionId", session_id);
  }

  std::string json_args;
  base::JSONWriter::Write(request, &json_args);
  agent_host_->DispatchProtocolMessage(
      this, base::as_bytes(base::make_span(json_args)));

  return handle;
}

void Debugger::ClearPendingRequests() {
  for (auto& it : pending_requests_)
    it.second.RejectWithErrorMessage("target closed while handling command");
  pending_requests_.clear();
}

// static
gin::Handle<Debugger> Debugger::Create(v8::Isolate* isolate,
                                       content::WebContents* web_contents) {
  return gin::CreateHandle(isolate, new Debugger(isolate, web_contents));
}

gin::ObjectTemplateBuilder Debugger::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return gin_helper::EventEmitterMixin<Debugger>::GetObjectTemplateBuilder(
             isolate)
      .SetMethod("attach", &Debugger::Attach)
      .SetMethod("isAttached", &Debugger::IsAttached)
      .SetMethod("detach", &Debugger::Detach)
      .SetMethod("sendCommand", &Debugger::SendCommand);
}

const char* Debugger::GetTypeName() {
  return "Debugger";
}

}  // namespace api

}  // namespace electron
