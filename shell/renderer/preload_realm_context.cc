// Copyright (c) 2025 Salesforce, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/renderer/preload_realm_context.h"

#include "base/command_line.h"
#include "base/containers/span.h"
#include "base/process/process_metrics.h"
#include "mojo/public/cpp/base/big_buffer.h"
#include "shell/common/api/electron_bindings.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/js2c_bundle_ids.h"
#include "shell/common/node_util.h"
#include "shell/renderer/preload_environment.h"
#include "shell/renderer/preload_realm_global_scope.h"
#include "shell/renderer/preload_utils.h"
#include "shell/renderer/service_worker_data.h"
#include "third_party/blink/renderer/bindings/core/v8/script_state_impl.h"  // nogncheck
#include "third_party/blink/renderer/core/execution_context/execution_context.h"  // nogncheck
#include "third_party/blink/renderer/core/inspector/worker_thread_debugger.h"  // nogncheck
#include "third_party/blink/renderer/core/workers/worker_or_worklet_global_scope.h"  // nogncheck
#include "third_party/blink/renderer/platform/bindings/script_state.h"  // nogncheck
#include "third_party/blink/renderer/platform/bindings/v8_dom_wrapper.h"  // nogncheck
#include "third_party/blink/renderer/platform/bindings/v8_per_context_data.h"  // nogncheck
#include "third_party/blink/renderer/platform/context_lifecycle_observer.h"  // nogncheck
#include "third_party/blink/renderer/platform/supplementable.h"  // nogncheck
#include "v8/include/v8-context.h"

namespace electron::preload_realm {

namespace {

// This is a helper class to make the initiator ExecutionContext the owner
// of a PreloadRealmGlobalScope and its ScriptState. When the initiator
// ExecutionContext is destroyed, the PreloadRealmGlobalScope is destroyed,
// too.
class PreloadRealmLifetimeController
    : public blink::GarbageCollected<PreloadRealmLifetimeController>,
      public blink::Supplement<blink::ExecutionContext>,
      public blink::ContextLifecycleObserver {
 public:
  static const char kSupplementName[];

  explicit PreloadRealmLifetimeController(
      blink::ExecutionContext* initiator_execution_context,
      blink::ScriptState* initiator_script_state,
      blink::PreloadRealmGlobalScope* preload_realm_global_scope,
      blink::ScriptState* preload_realm_script_state,
      electron::ServiceWorkerData* service_worker_data)
      : blink::Supplement<blink::ExecutionContext>(*preload_realm_global_scope),
        initiator_script_state_(initiator_script_state),
        is_initiator_worker_or_worklet_(
            initiator_execution_context->IsWorkerOrWorkletGlobalScope()),
        preload_realm_global_scope_(preload_realm_global_scope),
        preload_realm_script_state_(preload_realm_script_state),
        service_worker_data_(service_worker_data) {
    SetContextLifecycleNotifier(initiator_execution_context);
    RegisterDebugger(initiator_execution_context);

    blink::Supplement<blink::ExecutionContext>::ProvideTo(
        *initiator_execution_context, this);
    blink::Supplement<blink::ExecutionContext>::ProvideTo(
        *preload_realm_global_scope, this);

    metrics_ = base::ProcessMetrics::CreateCurrentProcessMetrics();
    RunInitScript();
  }

  static PreloadRealmLifetimeController* From(v8::Local<v8::Context> context) {
    if (context.IsEmpty())
      return nullptr;
    return blink::Supplement<blink::ExecutionContext>::From<
        PreloadRealmLifetimeController>(blink::ExecutionContext::From(context));
  }

  void Trace(blink::Visitor* visitor) const override {
    visitor->Trace(initiator_script_state_);
    visitor->Trace(preload_realm_global_scope_);
    visitor->Trace(preload_realm_script_state_);
    blink::Supplement<blink::ExecutionContext>::Trace(visitor);
    ContextLifecycleObserver::Trace(visitor);
  }

  bool IsPreloadRealmContext(v8::Local<v8::Context> context) const {
    return preload_realm_script_state_ &&
           preload_realm_script_state_->ContextIsValid() &&
           preload_realm_script_state_->GetContext() == context;
  }

  v8::MaybeLocal<v8::Context> GetContext() {
    return preload_realm_script_state_ &&
                   preload_realm_script_state_->ContextIsValid()
               ? preload_realm_script_state_->GetContext()
               : v8::MaybeLocal<v8::Context>();
  }

  v8::MaybeLocal<v8::Context> GetInitiatorContext() {
    return initiator_script_state_ && initiator_script_state_->ContextIsValid()
               ? initiator_script_state_->GetContext()
               : v8::MaybeLocal<v8::Context>();
  }

  electron::ServiceWorkerData* service_worker_data() {
    return service_worker_data_;
  }

 protected:
  void ContextDestroyed() override {
    v8::HandleScope handle_scope(realm_isolate());
    preload_realm_global_scope_
        ->blink::Supplementable<blink::ExecutionContext>::RemoveSupplement<
            PreloadRealmLifetimeController>();

    preload_realm_script_state_->DisposePerContextData();
    // No V8 GC runs during worker thread termination, so the ScriptState's
    // weak callback never detaches the context.
    if (is_initiator_worker_or_worklet_) {
      preload_realm_script_state_->DissociateContext();
    }
    preload_realm_script_state_.Clear();
    preload_realm_global_scope_->NotifyContextDestroyed();
    preload_realm_global_scope_.Clear();
  }

 private:
  v8::Isolate* realm_isolate() {
    return preload_realm_script_state_->GetIsolate();
  }
  v8::Local<v8::Context> realm_context() {
    return preload_realm_script_state_->GetContext();
  }

  void RegisterDebugger(blink::ExecutionContext* initiator_execution_context) {
    v8::Isolate* isolate = realm_isolate();
    v8::Local<v8::Context> context = realm_context();

    blink::WorkerThreadDebugger* debugger =
        blink::WorkerThreadDebugger::From(isolate);
    ;
    const auto* worker_context =
        To<blink::WorkerOrWorkletGlobalScope>(initiator_execution_context);

    // Override path to make preload realm easier to find in debugger.
    blink::KURL url_for_debugger(worker_context->Url());
    url_for_debugger.SetPath("electron-preload-realm");

    debugger->ContextCreated(worker_context->GetThread(), url_for_debugger,
                             context);
  }

  void RunInitScript() {
    v8::Isolate* isolate = realm_isolate();
    v8::Local<v8::Context> context = realm_context();

    v8::Context::Scope context_scope(context);
    v8::MicrotasksScope microtasks_scope(
        isolate, context->GetMicrotaskQueue(),
        v8::MicrotasksScope::kDoNotRunMicrotasks);

    // The browser attached the service-worker preload set + process info to
    // this worker's EmbeddedWorkerStartParams (see
    // ContentBrowserClient::GetServiceWorkerStartupData); Chromium marshalled
    // it onto the worker thread with the rest of the start params, ordered
    // with worker creation. Deserialize and capture it once on the
    // ServiceWorkerData so preload contents and code caches are looked up
    // from it without marshaling them through V8.
    if (const std::optional<mojo_base::BigBuffer>& blob =
            service_worker_data_->proxy()->ElectronPreloadData()) {
      mojom::RendererStartupDataPtr data;
      if (mojom::RendererStartupData::Deserialize(
              base::span<const uint8_t>(*blob), &data)) {
        service_worker_data_->SetWorkerStartupData(std::move(data));
      }
    }

    gin_helper::Dictionary process = gin::Dictionary::CreateEmpty(isolate);
    ElectronBindings::BindProcess(isolate, &process, metrics_.get());
    process.SetMethod<&preload_utils::Uptime>("uptime");
    process.Set("argv", base::CommandLine::ForCurrentProcess()->argv());
    process.Set("pid", base::GetCurrentProcId());
    process.Set("sandboxed", true);
    process.Set("type", "service-worker");
    process.Set("contextIsolated", true);

    // Create ipcRenderer up front so that messages from the browser have
    // somewhere to go before a preload script asks for it.
    preload_utils::GetBinding(
        isolate, gin::StringToV8(isolate, "electron_renderer_ipc"));

    const mojom::RendererStartupDataPtr& startup_data =
        service_worker_data_->worker_startup_data();
    v8::Local<v8::Object> preload_process =
        preload_environment::CreateProcessObject(context, process,
                                                 startup_data);
    v8::Local<v8::Object> electron_module =
        preload_environment::CreateElectronModule(
            context, preload_environment::Flavor::kServiceWorker);
    preload_environment::RunPreloadScripts(
        context, /*render_frame=*/nullptr, service_worker_data_.get(),
        preload_process, electron_module,
        preload_environment::Flavor::kServiceWorker, startup_data);
  }

  const blink::WeakMember<blink::ScriptState> initiator_script_state_;
  bool is_initiator_worker_or_worklet_;
  blink::Member<blink::PreloadRealmGlobalScope> preload_realm_global_scope_;
  blink::Member<blink::ScriptState> preload_realm_script_state_;

  std::unique_ptr<base::ProcessMetrics> metrics_;
  raw_ptr<ServiceWorkerData, DanglingUntriaged> service_worker_data_;
};

const char PreloadRealmLifetimeController::kSupplementName[] =
    "PreloadRealmLifetimeController";

}  // namespace

v8::MaybeLocal<v8::Context> GetInitiatorContext(
    v8::Local<v8::Context> context) {
  DCHECK(!context.IsEmpty());
  auto* controller = PreloadRealmLifetimeController::From(context);
  if (!controller || !controller->IsPreloadRealmContext(context))
    return v8::MaybeLocal<v8::Context>();
  return controller->GetInitiatorContext();
}

v8::MaybeLocal<v8::Context> GetPreloadRealmContext(
    v8::Local<v8::Context> context) {
  DCHECK(!context.IsEmpty());
  blink::ExecutionContext* execution_context =
      blink::ExecutionContext::From(context);
  if (!execution_context->IsServiceWorkerGlobalScope())
    return v8::MaybeLocal<v8::Context>();
  auto* controller = PreloadRealmLifetimeController::From(context);
  if (controller)
    return controller->GetContext();
  return v8::MaybeLocal<v8::Context>();
}

electron::ServiceWorkerData* GetServiceWorkerData(
    v8::Local<v8::Context> context) {
  auto* controller = PreloadRealmLifetimeController::From(context);
  return controller ? controller->service_worker_data() : nullptr;
}

void OnCreatePreloadableV8Context(
    v8::Isolate* const isolate,
    v8::Local<v8::Context> initiator_context,
    electron::ServiceWorkerData* service_worker_data) {
  blink::ScriptState* initiator_script_state =
      blink::ScriptState::MaybeFrom(isolate, initiator_context);
  DCHECK(initiator_script_state);
  blink::ExecutionContext* initiator_execution_context =
      blink::ExecutionContext::From(initiator_context);
  DCHECK(initiator_execution_context);
  // The preload realm is a second world on the worker's thread. Blink's
  // isolated world types are tied to frames on the main thread.
  blink::DOMWrapperWorld* world = blink::DOMWrapperWorld::Create(
      isolate, blink::DOMWrapperWorld::WorldType::kWorkerOrWorklet);
  CHECK(world);  // Not yet run out of the world id.

  // Create a new PreloadRealmGlobalScope.
  blink::PreloadRealmGlobalScope* preload_realm_global_scope =
      blink::MakeGarbageCollected<blink::PreloadRealmGlobalScope>(
          initiator_execution_context);
  const blink::WrapperTypeInfo* wrapper_type_info =
      preload_realm_global_scope->GetWrapperTypeInfo();

  // Create a new v8::Context.
  v8::Local<v8::ObjectTemplate> global_template =
      wrapper_type_info->GetV8ClassTemplate(isolate, *world)
          .As<v8::FunctionTemplate>()
          ->InstanceTemplate();
  v8::Local<v8::Object> global_proxy;  // Will request a new global proxy.
  v8::Local<v8::Context> context =
      v8::Context::New(isolate, nullptr, global_template, global_proxy,
                       v8::DeserializeInternalFieldsCallback(),
                       initiator_execution_context->GetMicrotaskQueue());
  context->UseDefaultSecurityToken();

  // Associate the Blink object with the v8::Context.
  blink::ScriptState* script_state =
      blink::MakeGarbageCollected<blink::ScriptStateImpl>(
          context, world, *preload_realm_global_scope);

  // Associate the Blink object with the v8::Objects.
  global_proxy = context->Global();
  blink::V8DOMWrapper::SetNativeInfoForGlobal(isolate, global_proxy,
                                              preload_realm_global_scope);

  // Install context-dependent properties.
  std::ignore =
      script_state->PerContextData()->ConstructorForType(wrapper_type_info);

  // Make the initiator execution context the owner of the
  // PreloadRealmGlobalScope and the ScriptState.
  blink::MakeGarbageCollected<PreloadRealmLifetimeController>(
      initiator_execution_context, initiator_script_state,
      preload_realm_global_scope, script_state, service_worker_data);
}

}  // namespace electron::preload_realm
