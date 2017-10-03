// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/renderer/atom_renderer_client.h"

#include <string>
#include <vector>

#include "atom/common/api/atom_bindings.h"
#include "atom/common/api/event_emitter_caller.h"
#include "atom/common/asar/asar_util.h"
#include "atom/common/atom_constants.h"
#include "atom/common/node_bindings.h"
#include "atom/common/options_switches.h"
#include "atom/renderer/api/atom_api_renderer_ipc.h"
#include "atom/renderer/atom_render_frame_observer.h"
#include "atom/renderer/atom_render_view_observer.h"
#include "atom/renderer/web_worker_observer.h"
#include "base/command_line.h"
#include "content/public/renderer/render_frame.h"
#include "native_mate/dictionary.h"
#include "third_party/WebKit/public/web/WebDocument.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"

#include "atom/common/node_includes.h"
#include "atom_natives.h"  // NOLINT: This file is generated with js2c

namespace atom {

namespace {

bool IsDevToolsExtension(content::RenderFrame* render_frame) {
  return static_cast<GURL>(render_frame->GetWebFrame()->GetDocument().Url())
      .SchemeIs("chrome-extension");
}

}  // namespace

AtomRendererClient::AtomRendererClient()
    : node_integration_initialized_(false),
      node_bindings_(NodeBindings::Create(NodeBindings::RENDERER)),
      atom_bindings_(new AtomBindings(uv_default_loop())) {
}

AtomRendererClient::~AtomRendererClient() {
  asar::ClearArchives();
}

void AtomRendererClient::RenderThreadStarted() {
  RendererClientBase::RenderThreadStarted();
}

void AtomRendererClient::RenderFrameCreated(
    content::RenderFrame* render_frame) {
  RendererClientBase::RenderFrameCreated(render_frame);
}

void AtomRendererClient::RenderViewCreated(content::RenderView* render_view) {
  new AtomRenderViewObserver(render_view, this);
  RendererClientBase::RenderViewCreated(render_view);
}

void AtomRendererClient::RunScriptsAtDocumentStart(
    content::RenderFrame* render_frame) {
  // Inform the document start pharse.
  node::Environment* env = node_bindings_->uv_env();
  if (env) {
    v8::HandleScope handle_scope(env->isolate());
    mate::EmitEvent(env->isolate(), env->process_object(), "document-start");
  }
}

void AtomRendererClient::RunScriptsAtDocumentEnd(
    content::RenderFrame* render_frame) {
  // Inform the document end pharse.
  node::Environment* env = node_bindings_->uv_env();
  if (env) {
    v8::HandleScope handle_scope(env->isolate());
    mate::EmitEvent(env->isolate(), env->process_object(), "document-end");
  }
}

void AtomRendererClient::DidCreateScriptContext(
    v8::Handle<v8::Context> context, content::RenderFrame* render_frame) {
  // Only allow node integration for the main frame, unless it is a devtools
  // extension page.
  if (!render_frame->IsMainFrame() && !IsDevToolsExtension(render_frame))
    return;

  // Prepare the node bindings.
  if (!node_integration_initialized_) {
    node_integration_initialized_ = true;
    node_bindings_->Initialize();
    node_bindings_->PrepareMessageLoop();
  }

  // Setup node environment for each window.
  node::Environment* env = node_bindings_->CreateEnvironment(context);

  // Add Electron extended APIs.
  atom_bindings_->BindTo(env->isolate(), env->process_object());
  AddRenderBindings(env->isolate(), env->process_object());

  // Load everything.
  node_bindings_->LoadEnvironment(env);

  if (node_bindings_->uv_env() == nullptr) {
    // Make uv loop being wrapped by window context.
    node_bindings_->set_uv_env(env);

    // Give the node loop a run to make sure everything is ready.
    node_bindings_->RunMessageLoop();
  }
}

void AtomRendererClient::WillReleaseScriptContext(
    v8::Handle<v8::Context> context, content::RenderFrame* render_frame) {
  // Only allow node integration for the main frame, unless it is a devtools
  // extension page.
  if (!render_frame->IsMainFrame() && !IsDevToolsExtension(render_frame))
    return;

  node::Environment* env = node::Environment::GetCurrent(context);
  if (env)
    mate::EmitEvent(env->isolate(), env->process_object(), "exit");

  // The main frame may be replaced.
  if (env == node_bindings_->uv_env())
    node_bindings_->set_uv_env(nullptr);

  // Destroy the node environment.
  // This is disabled because pending async tasks may still use the environment
  // and would cause crashes later. Node does not seem to clear all async tasks
  // when the environment is destroyed.
  // node::FreeEnvironment(env);

  // AtomBindings is tracking node environments.
  atom_bindings_->EnvironmentDestroyed(env);
}

bool AtomRendererClient::ShouldFork(blink::WebLocalFrame* frame,
                                    const GURL& url,
                                    const std::string& http_method,
                                    bool is_initial_navigation,
                                    bool is_server_redirect,
                                    bool* send_referrer) {
  // Handle all the navigations and reloads in browser.
  // FIXME We only support GET here because http method will be ignored when
  // the OpenURLFromTab is triggered, which means form posting would not work,
  // we should solve this by patching Chromium in future.
  *send_referrer = true;
  return http_method == "GET";
}

void AtomRendererClient::DidInitializeWorkerContextOnWorkerThread(
    v8::Local<v8::Context> context) {
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kNodeIntegrationInWorker)) {
    WebWorkerObserver::GetCurrent()->ContextCreated(context);
  }
}

void AtomRendererClient::WillDestroyWorkerContextOnWorkerThread(
    v8::Local<v8::Context> context) {
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kNodeIntegrationInWorker)) {
    WebWorkerObserver::GetCurrent()->ContextWillDestroy(context);
  }
}

void AtomRendererClient::SetupMainWorldOverrides(
    v8::Handle<v8::Context> context) {
  // Setup window overrides in the main world context
  v8::Isolate* isolate = context->GetIsolate();

  // Wrap the bundle into a function that receives the binding object as
  // an argument.
  std::string bundle(node::isolated_bundle_data,
      node::isolated_bundle_data + sizeof(node::isolated_bundle_data));
  std::string wrapper = "(function (binding, require) {\n" + bundle + "\n})";
  auto script = v8::Script::Compile(
      mate::ConvertToV8(isolate, wrapper)->ToString());
  auto func = v8::Handle<v8::Function>::Cast(
      script->Run(context).ToLocalChecked());

  auto binding = v8::Object::New(isolate);
  api::Initialize(binding, v8::Null(isolate), context, nullptr);

  // Pass in CLI flags needed to setup window
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  mate::Dictionary dict(isolate, binding);
  if (command_line->HasSwitch(switches::kGuestInstanceID))
    dict.Set(options::kGuestInstanceID,
             command_line->GetSwitchValueASCII(switches::kGuestInstanceID));
  if (command_line->HasSwitch(switches::kOpenerID))
    dict.Set(options::kOpenerID,
             command_line->GetSwitchValueASCII(switches::kOpenerID));
  dict.Set("hiddenPage", command_line->HasSwitch(switches::kHiddenPage));
  dict.Set("nativeWindowOpen",
           command_line->HasSwitch(switches::kNativeWindowOpen));

  v8::Local<v8::Value> args[] = { binding };
  ignore_result(func->Call(context, v8::Null(isolate), 1, args));
}


}  // namespace atom
