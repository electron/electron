// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/renderer/electron_sandboxed_renderer_client.h"

#include <iterator>
#include <vector>

#include "base/base_paths.h"
#include "base/command_line.h"
#include "base/process/process_metrics.h"
#include "chrome/common/chrome_version.h"
#include "content/public/renderer/render_frame.h"
#include "electron/buildflags/buildflags.h"
#include "electron/electron_version.h"
#include "shell/common/api/electron_bindings.h"
#include "shell/common/application_info.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/node_includes.h"
#include "shell/common/node_util.h"
#include "shell/common/options_switches.h"
#include "shell/renderer/electron_api_service_impl.h"
#include "shell/renderer/electron_render_frame_observer.h"
#include "shell/renderer/preload_realm_context.h"
#include "shell/renderer/preload_utils.h"
#include "shell/renderer/service_worker_data.h"
#include "third_party/blink/public/common/web_preferences/web_preferences.h"
#include "third_party/blink/public/platform/scheduler/web_agent_group_scheduler.h"
#include "third_party/blink/public/web/blink.h"
#include "third_party/blink/public/web/web_document.h"
#include "third_party/electron_node/src/node_binding.h"
#include "third_party/electron_node/src/node_metadata.h"

namespace electron {

namespace {

// Data which only lives on the service worker's thread
constinit thread_local ServiceWorkerData* service_worker_data = nullptr;

constexpr std::string_view kEmitProcessEventKey = "emit-process-event";

void InvokeEmitProcessEvent(v8::Isolate* const isolate,
                            v8::Local<v8::Context> context,
                            const std::string& event_name) {
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

namespace {

// Builds the { node, v8, uv, electron, chrome, ... } versions object the same
// way the browser process does in OnNodePreload (node_bindings.cc), but
// without needing a node::Environment — the values are static per-process
// metadata baked into the binary.
v8::Local<v8::Value> BuildVersions(v8::Isolate* isolate) {
  auto versions = gin_helper::Dictionary::CreateEmpty(isolate);
  for (const auto& [key, value] : node::per_process::metadata.versions.pairs())
    versions.SetReadOnly(key, value);
  versions.SetReadOnly(ELECTRON_PROJECT_NAME, ELECTRON_VERSION_STRING);
  versions.SetReadOnly("chrome", CHROME_VERSION_STRING);
#if BUILDFLAG(HAS_VENDOR_VERSION)
  versions.SetReadOnly(BUILDFLAG(VENDOR_VERSION_NAME),
                       BUILDFLAG(VENDOR_VERSION_VALUE));
#endif
  return versions.GetHandle();
}

// Converts the preload data the browser pushed via ElectronFrameStartup into
// the same shape lib/sandboxed_renderer/init.ts expects from the legacy
// BROWSER_SANDBOX_LOAD reply — { preloadScripts, process }. Returns an empty
// MaybeLocal if no startup data has been pushed yet (e.g. the initial empty
// document of a fresh RenderFrame, where the renderer falls back to sync IPC).
v8::MaybeLocal<v8::Value> BuildStartupData(
    v8::Isolate* isolate,
    const mojom::RendererStartupDataPtr& data) {
  if (!data)
    return {};

  auto out = gin_helper::Dictionary::CreateEmpty(isolate);

  // preloadScripts: [{ id, type, filePath, contents, error }]
  v8::LocalVector<v8::Value> scripts(isolate);
  scripts.reserve(data->preload_scripts.size());
  for (const auto& ps : data->preload_scripts) {
    auto entry = gin_helper::Dictionary::CreateEmpty(isolate);
    entry.Set("id", ps->id);
    entry.Set("type", ps->type);
    entry.Set("filePath", ps->file_path);
    // BigBuffer uses inline bytes below 64 KB and shared memory above; the
    // implicit base::span conversion handles both backing stores. byte_span()
    // would CHECK-fail for the shared-memory case.
    base::span<const uint8_t> bytes = ps->contents;
    entry.Set("contents",
              std::string_view(reinterpret_cast<const char*>(bytes.data()),
                               bytes.size()));
    // Match the legacy IPC handler shape: `error` is an Error object when the
    // file read failed (the legacy path serializes the fs.readFile error
    // through the IPC), and absent otherwise. preload.ts destructures
    // `{contents, error}` and checks `if (contents)... else if (error)...`.
    if (ps->error) {
      entry.Set("error", v8::Local<v8::Value>(v8::Exception::Error(
                             gin::StringToV8(isolate, *ps->error))));
    }
    scripts.push_back(entry.GetHandle());
  }
  out.Set("preloadScripts",
          v8::Array::New(isolate, scripts.data(), scripts.size()));

  // process: { arch, platform, env, version, versions, execPath } — same
  // shape as lib/browser/rpc-server.ts builds. arch/platform/version/versions
  // are filled from this binary's compiled-in metadata instead of being
  // shipped over the wire (they are identical in browser and renderer).
  auto proc = gin_helper::Dictionary::CreateEmpty(isolate);
  proc.Set("arch", node::per_process::metadata.arch);
  proc.Set("platform", node::per_process::metadata.platform);
  proc.Set("version", "v" + node::per_process::metadata.versions.node);
  proc.Set("versions", BuildVersions(isolate));
  auto env = gin_helper::Dictionary::CreateEmpty(isolate);
  for (const auto& [k, v] : data->environment)
    env.Set(k, v);
  proc.Set("env", env);
  proc.Set("execPath", data->helper_exec_path);
  out.Set("process", proc);

  return out.GetHandle();
}

}  // namespace

void ElectronSandboxedRendererClient::InitializeBindings(
    v8::Local<v8::Object> binding,
    v8::Isolate* const isolate,
    v8::Local<v8::Context> context,
    content::RenderFrame* render_frame) {
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

  // If the browser pushed startup data via ElectronFrameStartup ahead of this
  // navigation, hand it to the bundle so it can skip the BROWSER_SANDBOX_LOAD
  // sync IPC. If not, set startupData to null and the bundle falls back.
  auto* api_service = ElectronApiServiceImpl::Get(render_frame);
  v8::Local<v8::Value> startup_data;
  if (!api_service || !BuildStartupData(isolate, api_service->startup_data())
                           .ToLocal(&startup_data)) {
    startup_data = v8::Null(isolate);
  }
  b.Set("startupData", startup_data);
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
  if (!ShouldLoadPreload(isolate, context, render_frame))
    return;

  injected_frames_.insert(render_frame);

  // Wrap the bundle into a function that receives the binding object as
  // argument.
  auto binding = v8::Object::New(isolate);
  InitializeBindings(binding, isolate, context, render_frame);

  v8::LocalVector<v8::String> sandbox_preload_bundle_params(
      isolate, {node::FIXED_ONE_BYTE_STRING(isolate, "binding")});

  v8::LocalVector<v8::Value> sandbox_preload_bundle_args(isolate, {binding});

  util::CompileAndCall(
      isolate, isolate->GetCurrentContext(), "electron/js2c/sandbox_bundle",
      &sandbox_preload_bundle_params, &sandbox_preload_bundle_args);

  v8::HandleScope handle_scope{isolate};
  v8::Context::Scope context_scope{context};
  InvokeEmitProcessEvent(isolate, context, "loaded");
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
  InvokeEmitProcessEvent(isolate, context, "exit");
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

  InvokeEmitProcessEvent(isolate, context, event_name);
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

  auto* command_line = base::CommandLine::ForCurrentProcess();
  if (command_line->HasSwitch(switches::kServiceWorkerPreload)) {
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
