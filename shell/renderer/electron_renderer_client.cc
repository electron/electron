// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/renderer/electron_renderer_client.h"

#include <string>

#include "base/command_line.h"
#include "content/public/renderer/render_frame.h"
#include "electron/buildflags/buildflags.h"
#include "net/http/http_request_headers.h"
#include "shell/common/api/electron_bindings.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/event_emitter_caller.h"
#include "shell/common/node_bindings.h"
#include "shell/common/node_includes.h"
#include "shell/common/options_switches.h"
#include "shell/renderer/electron_render_frame_observer.h"
#include "shell/renderer/web_worker_observer.h"
#include "third_party/blink/public/common/web_preferences/web_preferences.h"
#include "third_party/blink/public/web/web_document.h"
#include "third_party/blink/public/web/web_local_frame.h"

namespace electron {

namespace {

bool IsDevToolsExtension(content::RenderFrame* render_frame) {
  return static_cast<GURL>(render_frame->GetWebFrame()->GetDocument().Url())
      .SchemeIs("chrome-extension");
}

}  // namespace

ElectronRendererClient::ElectronRendererClient()
    : node_bindings_(
          NodeBindings::Create(NodeBindings::BrowserEnvironment::kRenderer)),
      electron_bindings_(
          std::make_unique<ElectronBindings>(node_bindings_->uv_loop())) {}

ElectronRendererClient::~ElectronRendererClient() = default;

void ElectronRendererClient::RenderFrameCreated(
    content::RenderFrame* render_frame) {
  new ElectronRenderFrameObserver(render_frame, this);
  RendererClientBase::RenderFrameCreated(render_frame);
}

void ElectronRendererClient::RunScriptsAtDocumentStart(
    content::RenderFrame* render_frame) {
  RendererClientBase::RunScriptsAtDocumentStart(render_frame);
  // Inform the document start phase.
  v8::HandleScope handle_scope(v8::Isolate::GetCurrent());
  node::Environment* env = GetEnvironment(render_frame);
  if (env)
    gin_helper::EmitEvent(env->isolate(), env->process_object(),
                          "document-start");
}

void ElectronRendererClient::RunScriptsAtDocumentEnd(
    content::RenderFrame* render_frame) {
  RendererClientBase::RunScriptsAtDocumentEnd(render_frame);
  // Inform the document end phase.
  v8::HandleScope handle_scope(v8::Isolate::GetCurrent());
  node::Environment* env = GetEnvironment(render_frame);
  if (env)
    gin_helper::EmitEvent(env->isolate(), env->process_object(),
                          "document-end");
}

void ElectronRendererClient::DidCreateScriptContext(
    v8::Handle<v8::Context> renderer_context,
    content::RenderFrame* render_frame) {
  // TODO(zcbenz): Do not create Node environment if node integration is not
  // enabled.

  // Only load Node.js if we are a main frame or a devtools extension
  // unless Node.js support has been explicitly enabled for subframes.
  auto prefs = render_frame->GetBlinkPreferences();
  bool is_main_frame = render_frame->IsMainFrame();
  bool is_devtools = IsDevToolsExtension(render_frame);
  bool allow_node_in_subframes = prefs.node_integration_in_sub_frames;

  bool should_load_node =
      (is_main_frame || is_devtools || allow_node_in_subframes) &&
      !IsWebViewFrame(renderer_context, render_frame);
  if (!should_load_node)
    return;

  injected_frames_.insert(render_frame);

  if (!node_integration_initialized_) {
    node_integration_initialized_ = true;
    node_bindings_->Initialize();
    node_bindings_->PrepareMessageLoop();
  } else {
    node_bindings_->PrepareMessageLoop();
  }

  // Setup node tracing controller.
  if (!node::tracing::TraceEventHelper::GetAgent())
    node::tracing::TraceEventHelper::SetAgent(node::CreateAgent());

  // Setup node environment for each window.
  bool initialized = node::InitializeContext(renderer_context);
  CHECK(initialized);

  node::Environment* env =
      node_bindings_->CreateEnvironment(renderer_context, nullptr);

  // If we have disabled the site instance overrides we should prevent loading
  // any non-context aware native module.
  env->options()->force_context_aware = true;

  // We do not want to crash the renderer process on unhandled rejections.
  env->options()->unhandled_rejections = "warn";

  environments_.insert(env);

  // Add Electron extended APIs.
  electron_bindings_->BindTo(env->isolate(), env->process_object());
  gin_helper::Dictionary process_dict(env->isolate(), env->process_object());
  BindProcess(env->isolate(), &process_dict, render_frame);

  // Load everything.
  node_bindings_->LoadEnvironment(env);

  if (node_bindings_->uv_env() == nullptr) {
    // Make uv loop being wrapped by window context.
    node_bindings_->set_uv_env(env);

    // Give the node loop a run to make sure everything is ready.
    node_bindings_->RunMessageLoop();
  }
}

void ElectronRendererClient::WillReleaseScriptContext(
    v8::Handle<v8::Context> context,
    content::RenderFrame* render_frame) {
  if (injected_frames_.erase(render_frame) == 0)
    return;

  node::Environment* env = node::Environment::GetCurrent(context);
  if (environments_.erase(env) == 0)
    return;

  gin_helper::EmitEvent(env->isolate(), env->process_object(), "exit");

  // The main frame may be replaced.
  if (env == node_bindings_->uv_env())
    node_bindings_->set_uv_env(nullptr);

  // Destroy the node environment.  We only do this if node support has been
  // enabled for sub-frames to avoid a change-of-behavior / introduce crashes
  // for existing users.
  // We also do this if we have disable electron site instance overrides to
  // avoid memory leaks
  auto prefs = render_frame->GetBlinkPreferences();
  gin_helper::MicrotasksScope microtasks_scope(env->isolate());
  node::FreeEnvironment(env);
  if (env == node_bindings_->uv_env())
    node::FreeIsolateData(node_bindings_->isolate_data());

  // ElectronBindings is tracking node environments.
  electron_bindings_->EnvironmentDestroyed(env);
}

void ElectronRendererClient::WorkerScriptReadyForEvaluationOnWorkerThread(
    v8::Local<v8::Context> context) {
  // TODO(loc): Note that this will not be correct for in-process child windows
  // with webPreferences that have a different value for nodeIntegrationInWorker
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kNodeIntegrationInWorker)) {
    WebWorkerObserver::GetCurrent()->WorkerScriptReadyForEvaluation(context);
  }
}

void ElectronRendererClient::WillDestroyWorkerContextOnWorkerThread(
    v8::Local<v8::Context> context) {
  // TODO(loc): Note that this will not be correct for in-process child windows
  // with webPreferences that have a different value for nodeIntegrationInWorker
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kNodeIntegrationInWorker)) {
    WebWorkerObserver::GetCurrent()->ContextWillDestroy(context);
  }
}

node::Environment* ElectronRendererClient::GetEnvironment(
    content::RenderFrame* render_frame) const {
  if (injected_frames_.find(render_frame) == injected_frames_.end())
    return nullptr;
  v8::HandleScope handle_scope(v8::Isolate::GetCurrent());
  auto context =
      GetContext(render_frame->GetWebFrame(), v8::Isolate::GetCurrent());
  node::Environment* env = node::Environment::GetCurrent(context);
  if (environments_.find(env) == environments_.end())
    return nullptr;
  return env;
}

}  // namespace electron
