// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/electron_api_debugger.h"

#include <string>
#include <string_view>
#include <utility>

#include "base/containers/span.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "content/public/browser/devtools_agent_host.h"
#include "content/public/browser/web_contents.h"
#include "gin/object_template_builder.h"
#include "gin/per_isolate_data.h"
#include "shell/browser/javascript_environment.h"
#include "shell/common/gin_converters/value_converter.h"
#include "shell/common/gin_helper/handle.h"
#include "shell/common/gin_helper/promise.h"
#include "v8/include/cppgc/allocation.h"
#include "v8/include/v8-cppgc.h"

using content::DevToolsAgentHost;

namespace electron::api {

gin::WrapperInfo Debugger::kWrapperInfo = {{gin::kEmbedderNativeGin},
                                           gin::kElectronDebugger};

Debugger::Debugger(content::WebContents* web_contents)
    : content::WebContentsObserver{web_contents}, web_contents_{web_contents} {}

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
  v8::HandleScope handle_scope(isolate);

  const std::string_view message_str = base::as_string_view(message);
  std::optional<base::Value> parsed_message = base::JSONReader::Read(
      message_str, base::JSON_REPLACE_INVALID_CHARACTERS);
  if (!parsed_message || !parsed_message->is_dict())
    return;
  base::DictValue& dict = parsed_message->GetDict();
  std::optional<int> id = dict.FindInt("id");
  if (!id) {
    std::string* method = dict.FindString("method");
    if (!method)
      return;
    std::string* session_id = dict.FindString("sessionId");
    base::DictValue* params = dict.FindDict("params");
    Emit("message", *method, params ? std::move(*params) : base::DictValue(),
         session_id ? *session_id : "");
  } else {
    auto it = pending_requests_.find(*id);
    if (it == pending_requests_.end())
      return;

    gin_helper::Promise<base::DictValue> promise = std::move(it->second);
    pending_requests_.erase(it);

    base::DictValue* error = dict.FindDict("error");
    if (error) {
      std::string* error_message = error->FindString("message");
      promise.RejectWithErrorMessage(error_message ? *error_message : "");
    } else {
      base::DictValue* result = dict.FindDict("result");
      promise.Resolve(result ? std::move(*result) : base::DictValue());
    }
  }
}

void Debugger::RenderFrameHostChanged(content::RenderFrameHost* old_rfh,
                                      content::RenderFrameHost* new_rfh) {
  // ConnectWebContents uses the primary main frame of the webContents,
  // so if the new_rfh is not the primary main frame, we don't want to
  // reconnect otherwise we'll end up trying to reconnect to a RenderFrameHost
  // that already has a DevToolsAgentHost associated with it.
  if (agent_host_ && new_rfh->IsInPrimaryMainFrame()) {
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
  gin_helper::Promise<base::DictValue> promise(isolate);
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

  base::DictValue command_params;
  args->GetNext(&command_params);

  std::string session_id;
  if (args->GetNext(&session_id) && session_id.empty()) {
    promise.RejectWithErrorMessage("Empty session id is not allowed");
    return handle;
  }

  base::DictValue request;
  int request_id = ++previous_request_id_;
  pending_requests_.emplace(request_id, std::move(promise));
  request.Set("id", request_id);
  request.Set("method", method);
  if (!command_params.empty()) {
    request.Set("params", std::move(command_params));
  }

  if (!session_id.empty()) {
    request.Set("sessionId", session_id);
  }

  const auto json_args = base::WriteJson(request).value_or("");
  agent_host_->DispatchProtocolMessage(this, base::as_byte_span(json_args));

  return handle;
}

void Debugger::ClearPendingRequests() {
  for (auto& it : pending_requests_)
    it.second.RejectWithErrorMessage("target closed while handling command");
  pending_requests_.clear();
}

// static
Debugger* Debugger::Create(v8::Isolate* isolate,
                           content::WebContents* web_contents) {
  return cppgc::MakeGarbageCollected<Debugger>(
      isolate->GetCppHeap()->GetAllocationHandle(), web_contents);
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

const gin::WrapperInfo* Debugger::wrapper_info() const {
  return &kWrapperInfo;
}

const char* Debugger::GetHumanReadableName() const {
  return "Electron / Debugger";
}

}  // namespace electron::api
