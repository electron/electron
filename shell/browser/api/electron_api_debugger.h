// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_API_ELECTRON_API_DEBUGGER_H_
#define ELECTRON_SHELL_BROWSER_API_ELECTRON_API_DEBUGGER_H_

#include <map>
#include <memory>

#include "base/containers/span.h"
#include "base/values.h"
#include "content/public/browser/web_contents_observer.h"
#include "gin/wrappable.h"
#include "shell/browser/event_emitter_mixin.h"

namespace content {
class DevToolsAgentHost;
class WebContents;
}  // namespace content

namespace gin {
class Arguments;
}  // namespace gin

namespace gin_helper {
template <typename T>
class Promise;
}  // namespace gin_helper

namespace electron::api {

class Debugger final : public gin::Wrappable<Debugger>,
                       public gin_helper::EventEmitterMixin<Debugger>,
                       private content::WebContentsObserver {
 public:
  static Debugger* Create(v8::Isolate* isolate,
                          content::WebContents* web_contents);

  // Make public for cppgc::MakeGarbageCollected.
  Debugger(v8::Isolate* isolate, content::WebContents* web_contents);
  ~Debugger() override;

  // gin_helper::Wrappable
  static gin::WrapperInfo kWrapperInfo;
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;
  const gin::WrapperInfo* wrapper_info() const override;
  const char* GetHumanReadableName() const override;

  const char* GetClassName() const { return "Debugger"; }

  // disable copy
  Debugger(const Debugger&) = delete;
  Debugger& operator=(const Debugger&) = delete;

 protected:
  // content::WebContentsObserver:
  void RenderFrameHostChanged(content::RenderFrameHost* old_rfh,
                              content::RenderFrameHost* new_rfh) override;

 private:
  class AgentHostClient;
  using PendingRequestMap = std::map<int, gin_helper::Promise<base::DictValue>>;

  void AgentHostClosed();
  void DispatchProtocolMessage(base::span<const uint8_t> message);
  void Attach(gin::Arguments* args);
  bool IsAttached();
  void Detach();
  v8::Local<v8::Promise> SendCommand(gin::Arguments* args);
  void ClearPendingRequests();

  std::unique_ptr<AgentHostClient> agent_host_client_;

  PendingRequestMap pending_requests_;
  int previous_request_id_ = 0;
};

}  // namespace electron::api

#endif  // ELECTRON_SHELL_BROWSER_API_ELECTRON_API_DEBUGGER_H_
