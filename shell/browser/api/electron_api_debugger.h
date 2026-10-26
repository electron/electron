// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_API_ELECTRON_API_DEBUGGER_H_
#define ELECTRON_SHELL_BROWSER_API_ELECTRON_API_DEBUGGER_H_

#include <memory>
#include <string>

#include "base/values.h"
#include "gin/wrappable.h"
#include "shell/browser/event_emitter_mixin.h"
#include "shell/browser/native_peer.h"

namespace content {
class DevToolsAgentHost;
class WebContents;
}  // namespace content

namespace gin {
class Arguments;
}  // namespace gin

namespace electron::api {
class Debugger final : public gin::Wrappable<Debugger>,
                       public gin_helper::EventEmitterMixin<Debugger> {
 public:
  static Debugger* Create(v8::Isolate* isolate,
                          content::WebContents* web_contents);

  // Make public for cppgc::MakeGarbageCollected.
  explicit Debugger(content::WebContents* web_contents);
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

 private:
  class AgentHostLifecycle;

  void AgentHostClosed();
  void EmitProtocolMessage(const std::string& method,
                           base::DictValue params,
                           const std::string& session_id);
  void Attach(gin::Arguments* args);
  bool IsAttached();
  void Detach();
  v8::Local<v8::Promise> SendCommand(gin::Arguments* args);

  std::unique_ptr<AgentHostLifecycle, NativePeerBase::Deleter>
      agent_host_lifecycle_;
};

}  // namespace electron::api

#endif  // ELECTRON_SHELL_BROWSER_API_ELECTRON_API_DEBUGGER_H_
