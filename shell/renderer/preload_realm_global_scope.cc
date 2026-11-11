// Copyright (c) 2026 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/renderer/preload_realm_global_scope.h"

#include <utility>

#include "base/notreached.h"
#include "base/task/single_thread_task_runner.h"
#include "services/metrics/public/cpp/ukm_source_id.h"
#include "third_party/blink/public/platform/browser_interface_broker_proxy.h"
#include "third_party/blink/renderer/core/inspector/console_message.h"  // nogncheck
#include "third_party/blink/renderer/core/inspector/inspector_audits_issue.h"  // nogncheck

namespace blink {

PreloadRealmGlobalScope::PreloadRealmGlobalScope(
    ExecutionContext* initiator_execution_context)
    : ExecutionContext(initiator_execution_context->GetIsolate(),
                       initiator_execution_context->GetAgent()),
      initiator_execution_context_(initiator_execution_context) {}

void PreloadRealmGlobalScope::Trace(Visitor* visitor) const {
  visitor->Trace(initiator_execution_context_);
  ScriptWrappable::Trace(visitor);
  UniversalGlobalScope::Trace(visitor);
  ExecutionContext::Trace(visitor);
}

ExecutionContext* PreloadRealmGlobalScope::GetExecutionContext() const {
  return const_cast<PreloadRealmGlobalScope*>(this);
}

// Preload scripts reach the browser through Electron's IPC, never through
// Blink's own interfaces.
const BrowserInterfaceBrokerProxy&
PreloadRealmGlobalScope::GetBrowserInterfaceBroker() const {
  return GetEmptyBrowserInterfaceBroker();
}

scoped_refptr<base::SingleThreadTaskRunner>
PreloadRealmGlobalScope::GetTaskRunner(TaskType task_type) {
  return initiator_execution_context_->GetTaskRunner(task_type);
}

// Preload scripts are app code, so their feature usage is not counted as the
// initiator's.
void PreloadRealmGlobalScope::CountUse(mojom::blink::WebFeature feature) {}

void PreloadRealmGlobalScope::CountDeprecation(
    mojom::blink::WebFeature feature) {}

void PreloadRealmGlobalScope::CountWebDXFeature(
    mojom::blink::WebDXFeature feature) {}

// Return true so V8ScriptValueDeserializer::ExecutionContextExposesInterface
// allows structuredClone() of DOMException in preload realms without patching
// dom_exception.idl.
bool PreloadRealmGlobalScope::IsShadowRealmGlobalScope() const {
  return true;
}

ExecutionContext* PreloadRealmGlobalScope::GetRootExecutionContext() {
  return initiator_execution_context_->GetRootExecutionContext();
}

const KURL& PreloadRealmGlobalScope::Url() const {
  return initiator_execution_context_->Url();
}

const KURL& PreloadRealmGlobalScope::BaseURL() const {
  return initiator_execution_context_->BaseURL();
}

KURL PreloadRealmGlobalScope::CompleteURL(const String& url) const {
  return initiator_execution_context_->CompleteURL(url);
}

// No Content Security Policy is ever bound to a preload realm, so nothing
// can ask it to disable eval.
void PreloadRealmGlobalScope::DisableEval(const String& error_message) {
  NOTREACHED();
}

void PreloadRealmGlobalScope::SetWasmEvalErrorMessage(
    const String& error_message) {
  NOTREACHED();
}

String PreloadRealmGlobalScope::UserAgent() const {
  return initiator_execution_context_->UserAgent();
}

HttpsState PreloadRealmGlobalScope::GetHttpsState() const {
  return initiator_execution_context_->GetHttpsState();
}

// No interface that loads resources is exposed to a preload realm.
ResourceFetcher* PreloadRealmGlobalScope::Fetcher() {
  NOTREACHED();
}

void PreloadRealmGlobalScope::ExceptionThrown(
    ErrorEvent* error_event,
    SanitizeScriptErrors sanitize_script_errors) {
  initiator_execution_context_->ExceptionThrown(error_event,
                                                sanitize_script_errors);
}

void PreloadRealmGlobalScope::AddInspectorIssue(AuditsIssue issue) {
  initiator_execution_context_->AddInspectorIssue(std::move(issue));
}

// A preload realm has no `onerror`: uncaught errors go straight to
// ExceptionThrown().
EventTarget* PreloadRealmGlobalScope::ErrorEventTarget() {
  return nullptr;
}

FrameOrWorkerScheduler* PreloadRealmGlobalScope::GetScheduler() {
  return initiator_execution_context_->GetScheduler();
}

bool PreloadRealmGlobalScope::CrossOriginIsolatedCapability() const {
  return false;
}

bool PreloadRealmGlobalScope::IsIsolatedContext() const {
  return false;
}

ukm::UkmRecorder* PreloadRealmGlobalScope::UkmRecorder() {
  return nullptr;
}

ukm::SourceId PreloadRealmGlobalScope::UkmSourceID() const {
  return ukm::kInvalidSourceId;
}

// A preload realm identifies as its initiator, like an isolated world
// identifies as its frame. A token type of its own would have to be taught to
// every consumer of ExecutionContextToken in the browser process.
ExecutionContextToken PreloadRealmGlobalScope::GetExecutionContextToken()
    const {
  return initiator_execution_context_->GetExecutionContextToken();
}

bool PreloadRealmGlobalScope::IsSecureContext() const {
  return initiator_execution_context_->IsSecureContext();
}

void PreloadRealmGlobalScope::AddConsoleMessageImpl(ConsoleMessage* message,
                                                    bool discard_duplicates) {
  initiator_execution_context_->AddConsoleMessage(message, discard_duplicates);
}

}  // namespace blink
