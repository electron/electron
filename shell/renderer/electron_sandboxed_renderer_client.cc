// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/renderer/electron_sandboxed_renderer_client.h"

#include <iterator>
#include <tuple>
#include <vector>

#include "base/base_paths.h"
#include "base/command_line.h"
#include "base/containers/contains.h"
#include "base/process/process_metrics.h"
#include "content/public/renderer/render_frame.h"
#include "shell/common/api/electron_bindings.h"
#include "shell/common/application_info.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/microtasks_scope.h"
#include "shell/common/node_includes.h"
#include "shell/common/node_util.h"
#include "shell/common/options_switches.h"
#include "shell/renderer/electron_render_frame_observer.h"
#include "shell/renderer/preload_realm_context.h"
#include "shell/renderer/preload_utils.h"
#include "shell/renderer/service_worker_data.h"
#include "third_party/blink/public/common/web_preferences/web_preferences.h"
#include "third_party/blink/public/platform/scheduler/web_agent_group_scheduler.h"
#include "third_party/blink/public/web/blink.h"
#include "third_party/blink/public/web/web_document.h"
#include "third_party/electron_node/src/node_binding.h"

namespace electron {

namespace {

// Data which only lives on the service worker's thread
constinit thread_local ServiceWorkerData* service_worker_data = nullptr;

constexpr std::string_view kEmitProcessEventKey = "emit-process-event";

void InvokeEmitProcessEvent(v8::Local<v8::Context> context,
                            const std::string& event_name) {
  auto* isolate = context->GetIsolate();
  // set by sandboxed_renderer/init.js
  auto binding_key = gin::ConvertToV8(isolate, kEmitProcessEventKey)
                         ->ToString(context)
                         .ToLocalChecked();
  auto private_binding_key = v8::Private::ForApi(isolate, binding_key);
  auto global_object = context->Global();
  v8::Local<v8::Value> callback_value;
  if (!global_object->GetPrivate(context, private_binding_key)
           .ToLocal(&callback_value))
    return;
  if (callback_value.IsEmpty() || !callback_value->IsFunction())
    return;
  auto callback = callback_value.As<v8::Function>();
  v8::Local<v8::Value> args[] = {gin::ConvertToV8(isolate, event_name)};
  std::ignore =
      callback->Call(context, callback, std::size(args), std::data(args));
}

}  // namespace

ElectronSandboxedRendererClient::ElectronSandboxedRendererClient() {
  // Explicitly register electron's builtin bindings.
  NodeBindings::RegisterBuiltinBindings();
  metrics_ = base::ProcessMetrics::CreateCurrentProcessMetrics();
}

ElectronSandboxedRendererClient::~ElectronSandboxedRendererClient() = default;

void ElectronSandboxedRendererClient::InitializeBindings(
    v8::Local<v8::Object> binding,
    v8::Local<v8::Context> context,
    content::RenderFrame* render_frame) {
  auto* isolate = context->GetIsolate();
  gin_helper::Dictionary b(isolate, binding);
  b.SetMethod("get", preload_utils::GetBinding);
  b.SetMethod("createPreloadScript", preload_utils::CreatePreloadScript);

  auto process = gin_helper::Dictionary::CreateEmpty(isolate);
  b.Set("process", process);

  ElectronBindings::BindProcess(isolate, &process, metrics_.get());
  BindProcess(isolate, &process, render_frame);

  process.SetMethod("uptime", preload_utils::Uptime);
  process.Set("argv", base::CommandLine::ForCurrentProcess()->argv());
  process.SetReadOnly("pid", base::GetCurrentProcId());
  process.SetReadOnly("sandboxed", true);
  process.SetReadOnly("type", "renderer");
}

void ElectronSandboxedRendererClient::RenderFrameCreated(
    content::RenderFrame* render_frame) {
  new ElectronRenderFrameObserver(render_frame, this);
  RendererClientBase::RenderFrameCreated(render_frame);
}

void ElectronSandboxedRendererClient::RunScriptsAtDocumentStart(
    content::RenderFrame* render_frame) {
  RendererClientBase::RunScriptsAtDocumentStart(render_frame);
  EmitProcessEvent(render_frame, "document-start");
}

void ElectronSandboxedRendererClient::RunScriptsAtDocumentEnd(
    content::RenderFrame* render_frame) {
  RendererClientBase::RunScriptsAtDocumentEnd(render_frame);
  EmitProcessEvent(render_frame, "document-end");
}

void ElectronSandboxedRendererClient::DidCreateScriptContext(
    v8::Local<v8::Context> context,
    content::RenderFrame* render_frame) {
  // Only allow preload for the main frame or
  // For devtools we still want to run the preload_bundle script
  // Or when nodeSupport is explicitly enabled in sub frames
  if (!ShouldLoadPreload(context, render_frame))
    return;

  injected_frames_.insert(render_frame);

  // Wrap the bundle into a function that receives the binding object as
  // argument.
  auto* isolate = context->GetIsolate();
  auto binding = v8::Object::New(isolate);
  InitializeBindings(binding, context, render_frame);

  std::vector<v8::Local<v8::String>> sandbox_preload_bundle_params = {
      node::FIXED_ONE_BYTE_STRING(isolate, "binding")};

  std::vector<v8::Local<v8::Value>> sandbox_preload_bundle_args = {binding};

  util::CompileAndCall(
      isolate->GetCurrentContext(), "electron/js2c/sandbox_bundle",
      &sandbox_preload_bundle_params, &sandbox_preload_bundle_args);

  v8::HandleScope handle_scope(isolate);
  v8::Context::Scope context_scope(context);
  InvokeEmitProcessEvent(context, "loaded");
}

void ElectronSandboxedRendererClient::WillReleaseScriptContext(
    v8::Local<v8::Context> context,
    content::RenderFrame* render_frame) {
  if (injected_frames_.erase(render_frame) == 0)
    return;

  auto* isolate = context->GetIsolate();
  gin_helper::MicrotasksScope microtasks_scope{
      isolate, context->GetMicrotaskQueue(), false,
      v8::MicrotasksScope::kDoNotRunMicrotasks};
  v8::HandleScope handle_scope(isolate);
  v8::Context::Scope context_scope(context);
  InvokeEmitProcessEvent(context, "exit");
}

void ElectronSandboxedRendererClient::EmitProcessEvent(
    content::RenderFrame* render_frame,
    const char* event_name) {
  if (!base::Contains(injected_frames_, render_frame))
    return;

  blink::WebLocalFrame* frame = render_frame->GetWebFrame();
  v8::Isolate* isolate = frame->GetAgentGroupScheduler()->Isolate();
  v8::HandleScope handle_scope{isolate};

  v8::Local<v8::Context> context = GetContext(frame, isolate);
  gin_helper::MicrotasksScope microtasks_scope{
      isolate, context->GetMicrotaskQueue(), false,
      v8::MicrotasksScope::kDoNotRunMicrotasks};
  v8::Context::Scope context_scope(context);

  InvokeEmitProcessEvent(context, event_name);
}

void ElectronSandboxedRendererClient::WillEvaluateServiceWorkerOnWorkerThread(
    blink::WebServiceWorkerContextProxy* context_proxy,
    v8::Local<v8::Context> v8_context,
    int64_t service_worker_version_id,
    const GURL& service_worker_scope,
    const GURL& script_url,
    const blink::ServiceWorkerToken& service_worker_token) {
  RendererClientBase::WillEvaluateServiceWorkerOnWorkerThread(
      context_proxy, v8_context, service_worker_version_id,
      service_worker_scope, script_url, service_worker_token);

  auto* command_line = base::CommandLine::ForCurrentProcess();
  if (command_line->HasSwitch(switches::kServiceWorkerPreload)) {
    if (!service_worker_data) {
      service_worker_data = new ServiceWorkerData(
          context_proxy, service_worker_version_id, v8_context);
    }

    preload_realm::OnCreatePreloadableV8Context(v8_context,
                                                service_worker_data);
  }
}

void ElectronSandboxedRendererClient::
    WillDestroyServiceWorkerContextOnWorkerThread(
        v8::Local<v8::Context> context,
        int64_t service_worker_version_id,
        const GURL& service_worker_scope,
        const GURL& script_url) {
  if (service_worker_data) {
    DCHECK_EQ(service_worker_version_id,
              service_worker_data->service_worker_version_id());
    delete service_worker_data;
    service_worker_data = nullptr;
  }

  RendererClientBase::WillDestroyServiceWorkerContextOnWorkerThread(
      context, service_worker_version_id, service_worker_scope, script_url);
}

}  // namespace electron
