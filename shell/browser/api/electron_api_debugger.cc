// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/electron_api_debugger.h"

#include <map>
#include <string>
#include <string_view>
#include <utility>

#include "base/containers/span.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/memory/raw_ptr.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/devtools_agent_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "gin/arguments.h"
#include "gin/object_template_builder.h"
#include "shell/browser/microtasks_runner.h"
#include "shell/common/gin_converters/value_converter.h"
#include "shell/common/gin_helper/handle.h"
#include "shell/common/gin_helper/promise.h"
#include "shell/common/gin_helper/wrappable_pointer_tags.h"
#include "v8/include/cppgc/allocation.h"
#include "v8/include/cppgc/persistent.h"
#include "v8/include/v8-cppgc.h"

using content::DevToolsAgentHost;

namespace electron::api {

gin::WrapperInfo Debugger::kWrapperInfo =
    electron::MakeWrapperInfo(electron::kElectronDebugger);

class Debugger::AgentHostLifecycle final
    : public content::DevToolsAgentHostClient,
      public MicrotasksRunner::Observer,
      private content::WebContentsObserver {
 public:
  using PendingRequestMap = std::map<int, gin_helper::Promise<base::DictValue>>;

  AgentHostLifecycle(Debugger* debugger, content::WebContents* web_contents)
      : content::WebContentsObserver(web_contents), debugger_(debugger) {
    MicrotasksRunner::AddObserver(this);
  }

  ~AgentHostLifecycle() override {
    MicrotasksRunner::RemoveObserver(this);
    Detach();
  }

  bool Attach(scoped_refptr<DevToolsAgentHost> agent_host) {
    DCHECK(!agent_host_);
    agent_host_ = std::move(agent_host);
    if (agent_host_->AttachClient(this))
      return true;
    agent_host_ = nullptr;
    return false;
  }

  bool Detach() {
    scoped_refptr<DevToolsAgentHost> agent_host = std::move(agent_host_);
    const bool detached = agent_host && agent_host->DetachClient(this);
    ClearPendingRequests();
    return detached;
  }

  DevToolsAgentHost* agent_host() const { return agent_host_.get(); }
  content::WebContents* web_contents() const {
    return content::WebContentsObserver::web_contents();
  }

  bool IsAttached() const { return agent_host_ && agent_host_->IsAttached(); }
  void OnBeforeMicrotasksRunnerDispose() override {
    cppgc::Persistent<Debugger> debugger(debugger_.Get());
    debugger_.Clear();
    if (Detach() && debugger)
      debugger->AgentHostClosed();
  }

  void AgentHostClosed(DevToolsAgentHost* agent_host) override {
    DCHECK_EQ(agent_host, agent_host_.get());
    if (agent_host != agent_host_.get())
      return;
    agent_host_ = nullptr;
    ClearPendingRequests();
    if (auto* debugger = debugger_.Get())
      debugger->AgentHostClosed();
  }

  void DispatchProtocolMessage(DevToolsAgentHost* agent_host,
                               base::span<const uint8_t> message) override {
    if (agent_host != agent_host_.get())
      return;

    const std::string_view message_str = base::as_string_view(message);
    std::optional<base::Value> parsed_message = base::JSONReader::Read(
        message_str, base::JSON_REPLACE_INVALID_CHARACTERS);
    if (!parsed_message || !parsed_message->is_dict())
      return;
    base::DictValue& dict = parsed_message->GetDict();
    std::optional<int> id = dict.FindInt("id");
    if (!id) {
      Debugger* debugger = debugger_.Get();
      std::string* method = dict.FindString("method");
      if (!debugger || !method)
        return;
      std::string* session_id = dict.FindString("sessionId");
      base::DictValue* params = dict.FindDict("params");
      debugger->EmitProtocolMessage(
          *method, params ? std::move(*params) : base::DictValue(),
          session_id ? *session_id : "");
      return;
    }

    auto it = pending_requests_.find(*id);
    if (it == pending_requests_.end())
      return;

    gin_helper::Promise<base::DictValue> promise = std::move(it->second);
    pending_requests_.erase(it);

    if (base::DictValue* error = dict.FindDict("error")) {
      std::string* error_message = error->FindString("message");
      promise.RejectWithErrorMessage(error_message ? *error_message : "");
    } else {
      base::DictValue* result = dict.FindDict("result");
      if (result) {
        promise.Resolve(*result);
      } else {
        promise.Resolve(base::DictValue());
      }
    }
  }

  void SendCommand(std::string method,
                   base::DictValue command_params,
                   std::string session_id,
                   gin_helper::Promise<base::DictValue> promise) {
    if (!agent_host_) {
      promise.RejectWithErrorMessage("No target available");
      return;
    }

    base::DictValue request;
    int request_id = ++previous_request_id_;
    pending_requests_.emplace(request_id, std::move(promise));
    request.Set("id", request_id);
    request.Set("method", method);
    if (!command_params.empty())
      request.Set("params", std::move(command_params));
    if (!session_id.empty())
      request.Set("sessionId", session_id);

    const auto json_args = base::WriteJson(request).value_or("");
    agent_host_->DispatchProtocolMessage(this, base::as_byte_span(json_args));
  }

 private:
  void RenderFrameHostChanged(content::RenderFrameHost* old_rfh,
                              content::RenderFrameHost* new_rfh) override {
    if (!agent_host_ || !new_rfh->IsInPrimaryMainFrame())
      return;

    auto* web_contents = content::WebContents::FromRenderFrameHost(new_rfh);

    // The agent host already follows primary main-frame changes within the
    // same WebContents. Reconnecting would tear down the session pipe and can
    // discard protocol notifications emitted during a RenderDocument swap.
    if (agent_host_->GetWebContents() == web_contents)
      return;

    agent_host_->DisconnectWebContents();
    agent_host_->ConnectWebContents(web_contents);
  }

  void ClearPendingRequests() {
    PendingRequestMap pending_requests = std::move(pending_requests_);
    for (auto& [id, promise] : pending_requests)
      promise.RejectWithErrorMessage("target closed while handling command");
  }

  cppgc::WeakPersistent<Debugger> debugger_;
  scoped_refptr<DevToolsAgentHost> agent_host_;
  PendingRequestMap pending_requests_;
  int previous_request_id_ = 0;
};

Debugger::Debugger(content::WebContents* web_contents)
    : agent_host_lifecycle_(
          new AgentHostLifecycle(this, web_contents),
          base::OnTaskRunnerDeleter(content::GetUIThreadTaskRunner({}))) {}

Debugger::~Debugger() = default;

void Debugger::AgentHostClosed() {
  Emit("detach", "target closed");
}

void Debugger::EmitProtocolMessage(const std::string& method,
                                   base::DictValue params,
                                   const std::string& session_id) {
  Emit("message", method, std::move(params), session_id);
}

void Debugger::Attach(gin::Arguments* args) {
  std::string protocol_version;
  args->GetNext(&protocol_version);

  if (agent_host_lifecycle_->agent_host()) {
    args->ThrowTypeError("Debugger is already attached to the target");
    return;
  }

  if (!protocol_version.empty() &&
      !DevToolsAgentHost::IsSupportedProtocolVersion(protocol_version)) {
    args->ThrowTypeError("Requested protocol version is not supported");
    return;
  }

  // The native client observes the target and clears this pointer when the
  // observed WebContents is destroyed.
  content::WebContents* web_contents = agent_host_lifecycle_->web_contents();
  if (!web_contents) {
    args->ThrowTypeError("No target available");
    return;
  }

  scoped_refptr<DevToolsAgentHost> agent_host =
      DevToolsAgentHost::GetOrCreateFor(web_contents);
  if (!agent_host) {
    args->ThrowTypeError("No target available");
    return;
  }

  if (!agent_host_lifecycle_->Attach(std::move(agent_host)))
    args->ThrowTypeError("Failed to attach debugger to the target");
}

bool Debugger::IsAttached() {
  return agent_host_lifecycle_->IsAttached();
}

void Debugger::Detach() {
  if (!agent_host_lifecycle_->Detach())
    return;
  AgentHostClosed();
}

v8::Local<v8::Promise> Debugger::SendCommand(gin::Arguments* args) {
  gin_helper::Promise<base::DictValue> promise(args->isolate());
  v8::Local<v8::Promise> handle = promise.GetHandle();

  if (!agent_host_lifecycle_->agent_host()) {
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

  agent_host_lifecycle_->SendCommand(std::move(method),
                                     std::move(command_params),
                                     std::move(session_id), std::move(promise));
  return handle;
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
