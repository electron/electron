// Copyright (c) 2026 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_RENDERER_PRELOAD_REALM_GLOBAL_SCOPE_H_
#define ELECTRON_SHELL_RENDERER_PRELOAD_REALM_GLOBAL_SCOPE_H_

#include "base/task/single_thread_task_runner.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"  // nogncheck
#include "third_party/blink/renderer/core/frame/universal_global_scope.h"  // nogncheck
#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"  // nogncheck
#include "third_party/blink/renderer/platform/weborigin/kurl.h"  // nogncheck

namespace blink {

// Global scope of the context Electron runs preload scripts in when the
// context they are attached to (the "initiator") has no isolated worlds of its
// own, as is the case for service workers. It exposes only the interfaces
// marked [Exposed=PreloadRealm] or [Exposed=*] and otherwise defers to the
// initiator, which owns it.
class PreloadRealmGlobalScope final : public ScriptWrappable,
                                      public UniversalGlobalScope,
                                      public ExecutionContext {
  DEFINE_WRAPPERTYPEINFO();

 public:
  explicit PreloadRealmGlobalScope(
      ExecutionContext* initiator_execution_context);

  ExecutionContext* GetInitiatorExecutionContext() const {
    return initiator_execution_context_.Get();
  }

  void Trace(Visitor* visitor) const override;

  // UniversalGlobalScope:
  ExecutionContext* GetExecutionContext() const override;

  // MojoBindingContext:
  const BrowserInterfaceBrokerProxy& GetBrowserInterfaceBroker() const override;
  scoped_refptr<base::SingleThreadTaskRunner> GetTaskRunner(
      TaskType task_type) override;

  // UseCounter:
  void CountUse(mojom::blink::WebFeature feature) override;
  void CountDeprecation(mojom::blink::WebFeature feature) override;
  void CountWebDXFeature(mojom::blink::WebDXFeature feature) override;

  // ExecutionContext:
  bool IsShadowRealmGlobalScope() const override;
  ExecutionContext* GetRootExecutionContext() override;
  const KURL& Url() const override;
  const KURL& BaseURL() const override;
  KURL CompleteURL(const String& url) const override;
  void DisableEval(const String& error_message) override;
  void SetWasmEvalErrorMessage(const String& error_message) override;
  String UserAgent() const override;
  HttpsState GetHttpsState() const override;
  ResourceFetcher* Fetcher() override;
  void ExceptionThrown(ErrorEvent* error_event,
                       SanitizeScriptErrors sanitize_script_errors) override;
  void AddInspectorIssue(AuditsIssue issue) override;
  EventTarget* ErrorEventTarget() override;
  FrameOrWorkerScheduler* GetScheduler() override;
  bool CrossOriginIsolatedCapability() const override;
  bool IsIsolatedContext() const override;
  ukm::UkmRecorder* UkmRecorder() override;
  ukm::SourceId UkmSourceID() const override;
  ExecutionContextToken GetExecutionContextToken() const override;
  bool IsSecureContext() const override;

 private:
  void AddConsoleMessageImpl(ConsoleMessage* message,
                             bool discard_duplicates) override;

  const Member<ExecutionContext> initiator_execution_context_;
};

}  // namespace blink

#endif  // ELECTRON_SHELL_RENDERER_PRELOAD_REALM_GLOBAL_SCOPE_H_
