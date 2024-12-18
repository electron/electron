// Copyright (c) 2024 Samuel Maddock <sam@samuelmaddock.com>.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/renderer/preload_realm_context.h"

#include "base/command_line.h"
#include "base/process/process.h"
#include "base/process/process_metrics.h"
#include "shell/common/api/electron_bindings.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/node_includes.h"
#include "shell/common/node_util.h"
#include "shell/renderer/preload_utils.h"
#include "shell/renderer/service_worker_data.h"
#include "third_party/blink/renderer/bindings/core/v8/script_controller.h"  // nogncheck
#include "third_party/blink/renderer/core/execution_context/execution_context.h"  // nogncheck
#include "third_party/blink/renderer/core/inspector/worker_thread_debugger.h"  // nogncheck
#include "third_party/blink/renderer/core/shadow_realm/shadow_realm_global_scope.h"  // nogncheck
#include "third_party/blink/renderer/core/workers/worker_or_worklet_global_scope.h"  // nogncheck
#include "third_party/blink/renderer/platform/bindings/script_state.h"  // nogncheck
#include "third_party/blink/renderer/platform/bindings/v8_dom_wrapper.h"  // nogncheck
#include "third_party/blink/renderer/platform/bindings/v8_per_context_data.h"  // nogncheck
#include "third_party/blink/renderer/platform/context_lifecycle_observer.h"  // nogncheck
#include "v8/include/v8-context.h"

namespace electron::preload_realm {

namespace {

static constexpr int kElectronContextEmbedderDataIndex =
    static_cast<int>(gin::kPerContextDataStartIndex) +
    static_cast<int>(gin::kEmbedderElectron);

// This is a helper class to make the initiator ExecutionContext the owner
// of a ShadowRealmGlobalScope and its ScriptState. When the initiator
// ExecutionContext is destroyed, the ShadowRealmGlobalScope is destroyed,
// too.
class PreloadRealmLifetimeController
    : public blink::GarbageCollected<PreloadRealmLifetimeController>,
      public blink::ContextLifecycleObserver {
 public:
  explicit PreloadRealmLifetimeController(
      blink::ExecutionContext* initiator_execution_context,
      blink::ScriptState* initiator_script_state,
      blink::ShadowRealmGlobalScope* shadow_realm_global_scope,
      blink::ScriptState* shadow_realm_script_state,
      electron::ServiceWorkerData* service_worker_data)
      : initiator_script_state_(initiator_script_state),
        is_initiator_worker_or_worklet_(
            initiator_execution_context->IsWorkerOrWorkletGlobalScope()),
        shadow_realm_global_scope_(shadow_realm_global_scope),
        shadow_realm_script_state_(shadow_realm_script_state),
        service_worker_data_(service_worker_data) {
    // Align lifetime of this controller to that of the initiator's context.
    self_ = this;

    SetContextLifecycleNotifier(initiator_execution_context);
    RegisterDebugger(initiator_execution_context);

    initiator_context()->SetAlignedPointerInEmbedderData(
        kElectronContextEmbedderDataIndex, static_cast<void*>(this));
    realm_context()->SetAlignedPointerInEmbedderData(
        kElectronContextEmbedderDataIndex, static_cast<void*>(this));

    metrics_ = base::ProcessMetrics::CreateCurrentProcessMetrics();
    RunInitScript();
  }

  static PreloadRealmLifetimeController* From(v8::Local<v8::Context> context) {
    if (context->GetNumberOfEmbedderDataFields() <=
        kElectronContextEmbedderDataIndex) {
      return nullptr;
    }
    auto* controller = static_cast<PreloadRealmLifetimeController*>(
        context->GetAlignedPointerFromEmbedderData(
            kElectronContextEmbedderDataIndex));
    CHECK(controller);
    return controller;
  }

  void Trace(blink::Visitor* visitor) const override {
    visitor->Trace(initiator_script_state_);
    visitor->Trace(shadow_realm_global_scope_);
    visitor->Trace(shadow_realm_script_state_);
    ContextLifecycleObserver::Trace(visitor);
  }

  v8::MaybeLocal<v8::Context> GetContext() {
    return shadow_realm_script_state_->ContextIsValid()
               ? shadow_realm_script_state_->GetContext()
               : v8::MaybeLocal<v8::Context>();
  }

  v8::MaybeLocal<v8::Context> GetInitiatorContext() {
    return initiator_script_state_->ContextIsValid()
               ? initiator_script_state_->GetContext()
               : v8::MaybeLocal<v8::Context>();
  }

  electron::ServiceWorkerData* service_worker_data() {
    return service_worker_data_;
  }

 protected:
  void ContextDestroyed() override {
    v8::HandleScope handle_scope(realm_isolate());
    realm_context()->SetAlignedPointerInEmbedderData(
        kElectronContextEmbedderDataIndex, nullptr);

    // See ShadowRealmGlobalScope::ContextDestroyed
    shadow_realm_script_state_->DisposePerContextData();
    if (is_initiator_worker_or_worklet_) {
      shadow_realm_script_state_->DissociateContext();
    }
    shadow_realm_script_state_.Clear();
    shadow_realm_global_scope_->NotifyContextDestroyed();
    shadow_realm_global_scope_.Clear();

    self_.Clear();
  }

 private:
  v8::Isolate* realm_isolate() {
    return shadow_realm_script_state_->GetIsolate();
  }
  v8::Local<v8::Context> realm_context() {
    return shadow_realm_script_state_->GetContext();
  }
  v8::Local<v8::Context> initiator_context() {
    return initiator_script_state_->GetContext();
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

    v8::Local<v8::Object> binding = v8::Object::New(isolate);

    gin_helper::Dictionary b(isolate, binding);
    b.SetMethod("get", preload_utils::GetBinding);
    b.SetMethod("createPreloadScript", preload_utils::CreatePreloadScript);

    gin_helper::Dictionary process = gin::Dictionary::CreateEmpty(isolate);
    b.Set("process", process);

    ElectronBindings::BindProcess(isolate, &process, metrics_.get());

    process.SetMethod("uptime", preload_utils::Uptime);
    process.Set("argv", base::CommandLine::ForCurrentProcess()->argv());
    process.SetReadOnly("pid", base::GetCurrentProcId());
    process.SetReadOnly("sandboxed", true);
    process.SetReadOnly("type", "service-worker");
    process.SetReadOnly("contextIsolated", true);

    std::vector<v8::Local<v8::String>> preload_realm_bundle_params = {
        node::FIXED_ONE_BYTE_STRING(isolate, "binding")};

    std::vector<v8::Local<v8::Value>> preload_realm_bundle_args = {binding};

    util::CompileAndCall(context, "electron/js2c/preload_realm_bundle",
                         &preload_realm_bundle_params,
                         &preload_realm_bundle_args);
  }

  const blink::WeakMember<blink::ScriptState> initiator_script_state_;
  bool is_initiator_worker_or_worklet_;
  blink::Member<blink::ShadowRealmGlobalScope> shadow_realm_global_scope_;
  blink::Member<blink::ScriptState> shadow_realm_script_state_;

  std::unique_ptr<base::ProcessMetrics> metrics_;
  raw_ptr<ServiceWorkerData> service_worker_data_;

  blink::Persistent<PreloadRealmLifetimeController> self_;
};

}  // namespace

v8::MaybeLocal<v8::Context> GetInitiatorContext(
    v8::Local<v8::Context> context) {
  DCHECK(!context.IsEmpty());
  blink::ExecutionContext* execution_context =
      blink::ExecutionContext::From(context);
  if (!execution_context->IsShadowRealmGlobalScope())
    return v8::MaybeLocal<v8::Context>();
  auto* controller = PreloadRealmLifetimeController::From(context);
  if (controller)
    return controller->GetInitiatorContext();
  return v8::MaybeLocal<v8::Context>();
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
    v8::Local<v8::Context> initiator_context,
    electron::ServiceWorkerData* service_worker_data) {
  v8::Isolate* isolate = initiator_context->GetIsolate();
  blink::ScriptState* initiator_script_state =
      blink::ScriptState::MaybeFrom(isolate, initiator_context);
  DCHECK(initiator_script_state);
  blink::ExecutionContext* initiator_execution_context =
      blink::ExecutionContext::From(initiator_context);
  DCHECK(initiator_execution_context);
  blink::DOMWrapperWorld* world = blink::DOMWrapperWorld::Create(
      isolate, blink::DOMWrapperWorld::WorldType::kShadowRealm);
  CHECK(world);  // Not yet run out of the world id.

  // Create a new ShadowRealmGlobalScope.
  blink::ShadowRealmGlobalScope* shadow_realm_global_scope =
      blink::MakeGarbageCollected<blink::ShadowRealmGlobalScope>(
          initiator_execution_context);
  const blink::WrapperTypeInfo* wrapper_type_info =
      shadow_realm_global_scope->GetWrapperTypeInfo();

  // Create a new v8::Context.
  // Initialize V8 extensions before creating the context.
  v8::ExtensionConfiguration extension_configuration =
      blink::ScriptController::ExtensionsFor(shadow_realm_global_scope);

  v8::Local<v8::ObjectTemplate> global_template =
      wrapper_type_info->GetV8ClassTemplate(isolate, *world)
          .As<v8::FunctionTemplate>()
          ->InstanceTemplate();
  v8::Local<v8::Object> global_proxy;  // Will request a new global proxy.
  v8::Local<v8::Context> context =
      v8::Context::New(isolate, &extension_configuration, global_template,
                       global_proxy, v8::DeserializeInternalFieldsCallback(),
                       initiator_execution_context->GetMicrotaskQueue());
  context->UseDefaultSecurityToken();

  // Associate the Blink object with the v8::Context.
  blink::ScriptState* script_state =
      blink::ScriptState::Create(context, world, shadow_realm_global_scope);

  // Associate the Blink object with the v8::Objects.
  global_proxy = context->Global();
  blink::V8DOMWrapper::SetNativeInfo(isolate, global_proxy,
                                     shadow_realm_global_scope);
  v8::Local<v8::Object> global_object =
      global_proxy->GetPrototype().As<v8::Object>();
  blink::V8DOMWrapper::SetNativeInfo(isolate, global_object,
                                     shadow_realm_global_scope);

  // Install context-dependent properties.
  std::ignore =
      script_state->PerContextData()->ConstructorForType(wrapper_type_info);

  // Make the initiator execution context the owner of the
  // ShadowRealmGlobalScope and the ScriptState.
  blink::MakeGarbageCollected<PreloadRealmLifetimeController>(
      initiator_execution_context, initiator_script_state,
      shadow_realm_global_scope, script_state, service_worker_data);
}

}  // namespace electron::preload_realm
