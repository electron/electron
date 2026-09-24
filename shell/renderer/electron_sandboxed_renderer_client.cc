// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/renderer/electron_sandboxed_renderer_client.h"

#include <iterator>

#include "base/command_line.h"
#include "base/process/process_metrics.h"
#include "content/public/renderer/render_frame.h"
#include "shell/common/api/electron_bindings.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/js2c_bundle_ids.h"
#include "shell/common/node_bindings.h"
#include "shell/common/node_util.h"
#include "shell/common/options_switches.h"
#include "shell/renderer/electron_api_service_impl.h"
#include "shell/renderer/electron_render_frame_observer.h"
#include "shell/renderer/preload_environment.h"
#include "shell/renderer/preload_realm_context.h"
#include "shell/renderer/preload_utils.h"
#include "shell/renderer/service_worker_data.h"
#include "third_party/blink/public/common/web_preferences/web_preferences.h"
#include "third_party/blink/public/platform/scheduler/web_agent_group_scheduler.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "v8/include/v8-function.h"

namespace electron {

namespace {

// Data which only lives on the service worker's thread
constinit thread_local ServiceWorkerData* service_worker_data = nullptr;

}  // namespace

ElectronSandboxedRendererClient::ElectronSandboxedRendererClient() {
  // Explicitly register electron's builtin bindings.
  NodeBindings::RegisterBuiltinBindings();
  metrics_ = base::ProcessMetrics::CreateCurrentProcessMetrics();
}

ElectronSandboxedRendererClient::~ElectronSandboxedRendererClient() = default;

void ElectronSandboxedRendererClient::SetUpPreloadEnvironment(
    v8::Isolate* isolate,
    v8::Local<v8::Context> context,
    content::RenderFrame* render_frame,
    const mojom::RendererStartupDataPtr& startup_data) {
  auto process = gin_helper::Dictionary::CreateEmpty(isolate);
  ElectronBindings::BindProcess(isolate, &process, metrics_.get());
  BindProcess(isolate, &process, render_frame);
  process.SetMethod("uptime", preload_utils::Uptime);
  process.Set("argv", base::CommandLine::ForCurrentProcess()->argv());
  process.Set("pid", base::GetCurrentProcId());
  process.Set("sandboxed", true);
  process.Set("type", "renderer");

  v8::Local<v8::Object> preload_process =
      preload_environment::CreateProcessObject(context, process, startup_data);
  v8::Local<v8::Object> electron_module =
      preload_environment::CreateElectronModule(
          context, preload_environment::Flavor::kRenderer);
  preload_environment::RunPreloadScripts(
      context, render_frame, /*service_worker_data=*/nullptr, preload_process,
      electron_module, preload_environment::Flavor::kRenderer, startup_data);
}

bool ElectronSandboxedRendererClient::HasScriptsToInject(
    content::RenderFrame* render_frame) const {
  // The <webview> element is implemented by the bundle in the embedder.
  if (render_frame->GetBlinkPreferences().webview_tag)
    return true;
  auto* api_service = ElectronApiServiceImpl::Get(render_frame);
  return api_service && api_service->startup_data() &&
         !api_service->startup_data()->preload_scripts.empty();
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
    v8::Isolate* const isolate,
    v8::Local<v8::Context> context,
    content::RenderFrame* render_frame) {
  RendererClientBase::DidCreateScriptContext(isolate, context, render_frame);

  // Only allow preload for the main frame or
  // For devtools we still want to run the preload_bundle script
  // Or when nodeSupport is explicitly enabled in sub frames
  if (!ShouldLoadPreload(isolate, context, render_frame) ||
      !HasScriptsToInject(render_frame)) {
    return;
  }

  injected_frames_.insert(render_frame);

  v8::HandleScope handle_scope{isolate};
  v8::Context::Scope context_scope{context};

  // Create ipcRenderer up front so that messages from the browser have
  // somewhere to go before a preload script asks for it.
  preload_utils::GetBinding(isolate,
                            gin::StringToV8(isolate, "electron_renderer_ipc"));

  // The <webview> element for renderers that enable it.
  const blink::web_pref::WebPreferences& prefs =
      render_frame->GetBlinkPreferences();
  if (prefs.webview_tag && render_frame->IsMainFrame()) {
    auto binding = gin_helper::Dictionary::CreateEmpty(isolate);
    binding.SetMethod("get", preload_utils::GetBinding);
    binding.Set("contextIsolated", prefs.context_isolation);
    v8::LocalVector<v8::String> params =
        js2c::MakeBundleParams(isolate, js2c::kWebViewBundleParams);
    v8::LocalVector<v8::Value> args(isolate, {binding.GetHandle()});
    util::CompileAndCall(isolate, context, js2c::kWebViewBundleId, &params,
                         &args);
  }

  // The browser pushed the preload script set and process info via
  // ElectronFrame, ordered ahead of the CommitNavigation that created this
  // context.
  auto* api_service = ElectronApiServiceImpl::Get(render_frame);
  if (api_service && api_service->startup_data() &&
      !api_service->startup_data()->preload_scripts.empty()) {
    SetUpPreloadEnvironment(isolate, context, render_frame,
                            api_service->startup_data());
  }

  preload_environment::EmitProcessEvent(context, "loaded");
}

void ElectronSandboxedRendererClient::WillReleaseScriptContext(
    v8::Isolate* const isolate,
    v8::Local<v8::Context> context,
    content::RenderFrame* render_frame) {
  if (injected_frames_.erase(render_frame) == 0)
    return;

  v8::MicrotasksScope microtasks_scope(
      context, v8::MicrotasksScope::kDoNotRunMicrotasks);
  v8::HandleScope handle_scope{isolate};
  v8::Context::Scope context_scope{context};
  preload_environment::EmitProcessEvent(context, "exit");
}

void ElectronSandboxedRendererClient::EmitProcessEvent(
    content::RenderFrame* render_frame,
    const char* event_name) {
  if (!injected_frames_.contains(render_frame))
    return;

  blink::WebLocalFrame* frame = render_frame->GetWebFrame();
  v8::Isolate* isolate = frame->GetAgentGroupScheduler()->Isolate();
  v8::HandleScope handle_scope{isolate};

  v8::Local<v8::Context> context = GetContext(frame, isolate);
  v8::MicrotasksScope microtasks_scope{
      context, v8::MicrotasksScope::kDoNotRunMicrotasks};
  v8::Context::Scope context_scope{context};

  preload_environment::EmitProcessEvent(context, event_name);
}

void ElectronSandboxedRendererClient::WillEvaluateServiceWorkerOnWorkerThread(
    blink::WebServiceWorkerContextProxy* context_proxy,
    v8::Isolate* const v8_isolate,
    v8::Local<v8::Context> v8_context,
    int64_t service_worker_version_id,
    const GURL& service_worker_scope,
    const GURL& script_url,
    const blink::ServiceWorkerToken& service_worker_token) {
  RendererClientBase::WillEvaluateServiceWorkerOnWorkerThread(
      context_proxy, v8_isolate, v8_context, service_worker_version_id,
      service_worker_scope, script_url, service_worker_token);

  // The browser attaches preload data only when the session has service
  // worker preload scripts registered (GetServiceWorkerStartupData).
  if (context_proxy->ElectronPreloadData().has_value()) {
    if (!service_worker_data) {
      service_worker_data = new ServiceWorkerData{
          context_proxy, service_worker_version_id, v8_isolate, v8_context};
    }

    preload_realm::OnCreatePreloadableV8Context(v8_isolate, v8_context,
                                                service_worker_data);
  }
}

void ElectronSandboxedRendererClient::
    WillDestroyServiceWorkerContextOnWorkerThread(
        v8::Local<v8::Context> context,
        int64_t service_worker_version_id,
        const GURL& service_worker_scope,
        const GURL& script_url,
        const blink::ServiceWorkerToken& service_worker_token) {
  if (service_worker_data) {
    DCHECK_EQ(service_worker_version_id,
              service_worker_data->service_worker_version_id());
    delete service_worker_data;
    service_worker_data = nullptr;
  }

  RendererClientBase::WillDestroyServiceWorkerContextOnWorkerThread(
      context, service_worker_version_id, service_worker_scope, script_url,
      service_worker_token);
}

}  // namespace electron
