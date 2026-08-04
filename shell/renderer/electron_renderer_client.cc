// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/renderer/electron_renderer_client.h"

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/task/sequenced_task_runner.h"
#include "content/public/renderer/render_frame.h"
#include "electron/fuses.h"
#include "net/http/http_request_headers.h"
#include "shell/common/api/electron_bindings.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/event_emitter_caller.h"
#include "shell/common/node_bindings.h"
#include "shell/common/node_includes.h"
#include "shell/common/node_util.h"
#include "shell/common/v8_util.h"
#include "shell/renderer/electron_render_frame_observer.h"
#include "shell/renderer/web_worker_observer.h"
#include "third_party/blink/public/common/web_preferences/web_preferences.h"
#include "third_party/blink/public/web/web_document.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_wasm_response_extensions.h"  // nogncheck
#include "third_party/blink/renderer/core/execution_context/execution_context.h"  // nogncheck
#include "third_party/blink/renderer/core/frame/web_local_frame_impl.h"  // nogncheck
#include "third_party/blink/renderer/core/workers/worker_global_scope.h"  // nogncheck
#include "third_party/blink/renderer/core/workers/worker_settings.h"  // nogncheck
#include "third_party/blink/renderer/core/workers/worklet_global_scope.h"  // nogncheck

namespace electron {

struct ElectronRendererClient::FrameEnvironment {
  // A main frame borrows |primary|, which integrates uv_default_loop(), while
  // no other environment occupies it; every other frame gets its own loop.
  FrameEnvironment(bool is_main_frame,
                   NodeBindings* primary,
                   ElectronBindings* primary_electron_bindings) {
    if (is_main_frame && primary->uv_env() == nullptr) {
      node_bindings = primary;
      electron_bindings = primary_electron_bindings;
      return;
    }
    own_node_bindings = NodeBindings::Create(
        NodeBindings::BrowserEnvironment::kRenderer, nullptr);
    own_electron_bindings =
        std::make_unique<ElectronBindings>(own_node_bindings->uv_loop());
    node_bindings = own_node_bindings.get();
    electron_bindings = own_electron_bindings.get();
  }

  std::unique_ptr<NodeBindings> own_node_bindings;
  std::unique_ptr<ElectronBindings> own_electron_bindings;
  raw_ptr<NodeBindings> node_bindings;
  raw_ptr<ElectronBindings> electron_bindings;
  std::shared_ptr<node::Environment> environment;
  base::WeakPtrFactory<FrameEnvironment> weak_factory{this};
};

ElectronRendererClient::ElectronRendererClient()
    : node_bindings_{
          NodeBindings::Create(NodeBindings::BrowserEnvironment::kRenderer,
                               uv_default_loop())},
      electron_bindings_{
          std::make_unique<ElectronBindings>(node_bindings_->uv_loop())} {}

ElectronRendererClient::~ElectronRendererClient() = default;

void ElectronRendererClient::PostIOThreadCreated(
    base::SingleThreadTaskRunner* io_thread_task_runner) {
  // Freezing flags after init conflicts with node in the renderer.
  // We do this here in order to avoid having to patch the ctor in
  // content/renderer/render_process_impl.cc.
  v8::V8::SetFlagsFromString("--no-freeze-flags-after-init");
}

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
  if (env) {
    v8::Context::Scope context_scope(env->context());
    gin_helper::EmitEvent(env->isolate(), env->process_object(),
                          "document-start");
  }
}

void ElectronRendererClient::RunScriptsAtDocumentEnd(
    content::RenderFrame* render_frame) {
  RendererClientBase::RunScriptsAtDocumentEnd(render_frame);
  // Inform the document end phase.
  v8::HandleScope handle_scope(v8::Isolate::GetCurrent());
  node::Environment* env = GetEnvironment(render_frame);
  if (env) {
    v8::Context::Scope context_scope(env->context());
    v8::MicrotasksScope microtasks_scope(env->isolate(),
                                         env->context()->GetMicrotaskQueue(),
                                         v8::MicrotasksScope::kRunMicrotasks);
    gin_helper::EmitEvent(env->isolate(), env->process_object(),
                          "document-end");
  }
}

void ElectronRendererClient::UndeferLoad(content::RenderFrame* render_frame) {
  render_frame->GetWebFrame()->GetDocumentLoader()->SetDefersLoading(
      blink::LoaderFreezeMode::kNone);
}

void ElectronRendererClient::DidCreateScriptContext(
    v8::Isolate* const isolate,
    v8::Local<v8::Context> renderer_context,
    content::RenderFrame* render_frame) {
  RendererClientBase::DidCreateScriptContext(isolate, renderer_context,
                                             render_frame);

  // TODO(zcbenz): Do not create Node environment if node integration is not
  // enabled.

  // Only load Node.js if we are a main frame or a devtools extension
  // unless Node.js support has been explicitly enabled for subframes.
  if (!ShouldLoadPreload(isolate, renderer_context, render_frame))
    return;

  if (!node_integration_initialized_) {
    node_integration_initialized_ = true;
    node_bindings_->Initialize(isolate, renderer_context);
    node_bindings_->SetUpIsolate(isolate);
    // SetUpIsolate registers Node's WebAssembly streaming callback, whose JS
    // side kNoBrowserGlobals never installs; put Blink's back.
    blink::WasmResponseExtensions::Initialize(isolate);
  }

  CHECK(!environments_.contains(render_frame));
  auto frame_env = std::make_unique<FrameEnvironment>(
      render_frame->IsMainFrame(), node_bindings_.get(),
      electron_bindings_.get());
  NodeBindings* node_bindings = frame_env->node_bindings;

  // Setup node tracing controller.
  NodeBindings::InitializeTracingAgent();

  // Setup node environment for each window.
  v8::Maybe<bool> initialized = node::InitializeContext(renderer_context);
  CHECK(!initialized.IsNothing() && initialized.FromJust());

  // Before we load the node environment, let's tell blink to hold off on
  // loading the body of this frame.  We will undefer the load once the preload
  // script has finished.  This allows our preload script to run async (E.g.
  // with ESM) without the preload being in a race
  render_frame->GetWebFrame()->GetDocumentLoader()->SetDefersLoading(
      blink::LoaderFreezeMode::kStrict);

  std::shared_ptr<node::Environment> env = node_bindings->CreateEnvironment(
      isolate, renderer_context, nullptr, 0,
      base::BindRepeating(&ElectronRendererClient::UndeferLoad,
                          base::Unretained(this), render_frame));
  frame_env->environment = env;
  node_bindings->set_uv_env(env.get());

  // If we have disabled the site instance overrides we should prevent loading
  // any non-context aware native module.
  env->options()->force_context_aware = true;

  // We do not want to crash the renderer process on unhandled rejections.
  env->options()->unhandled_rejections = "warn-with-error-code";

  // Add Electron extended APIs.
  frame_env->electron_bindings->BindTo(env->isolate(), env->process_object());
  gin_helper::Dictionary process_dict(env->isolate(), env->process_object());
  BindProcess(env->isolate(), &process_dict, render_frame);

  base::WeakPtr<FrameEnvironment> weak_frame_env =
      frame_env->weak_factory.GetWeakPtr();
  environments_[render_frame] = std::move(frame_env);

  node_bindings->LoadEnvironment(env.get());

  // This context may have been created from inside a script (e.g. the opener's
  // window.open() call), so give the loop its first run from a fresh task.
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(
                     [](base::WeakPtr<FrameEnvironment> frame_env) {
                       if (!frame_env)
                         return;
                       frame_env->node_bindings->PrepareEmbedThread();
                       frame_env->node_bindings->StartPolling();
                     },
                     weak_frame_env));
}

void ElectronRendererClient::WillReleaseScriptContext(
    v8::Isolate* const isolate,
    v8::Local<v8::Context> context,
    content::RenderFrame* render_frame) {
  node::Environment* env = GetEnvironment(render_frame);
  if (!env || env->context() != context)
    return;
  gin_helper::EmitEvent(isolate, env->process_object(), "exit");

  auto iter = environments_.find(render_frame);
  std::unique_ptr<FrameEnvironment> frame_env = std::move(iter->second);
  environments_.erase(iter);

  // Park the embed thread so FreeEnvironment's uv_run is the loop's only user.
  frame_env->node_bindings->set_uv_env(nullptr);
  frame_env->node_bindings->StopPolling();
  frame_env->electron_bindings->EnvironmentDestroyed(env);
  // Freeing the environment runs its loop, i.e. enters Node.js.
  util::ExplicitMicrotasksScope microtasks_scope(context->GetMicrotaskQueue());
  frame_env->environment.reset();
}

namespace {

bool WorkerHasNodeIntegration(blink::ExecutionContext* ec) {
  // We do not create a Node.js environment in service or shared workers
  // owing to an inability to customize sandbox policies in these workers
  // given that they're run out-of-process.
  // Also avoid creating a Node.js environment for worklet global scope
  // created on the main thread — those share the page's V8 context where
  // Node is already wired up.
  if (ec->IsServiceWorkerGlobalScope() || ec->IsSharedWorkerGlobalScope() ||
      ec->IsMainThreadWorkletGlobalScope())
    return false;

  // Off-main-thread worklets (AudioWorklet, PaintWorklet, AnimationWorklet,
  // SharedStorageWorklet) have their own dedicated worker thread but do not
  // derive from WorkerGlobalScope, so check for them separately and read the
  // flag from WorkletGlobalScope, which copies it out of the same
  // WorkerSettings as dedicated workers do.
  if (auto* wlgs = blink::DynamicTo<blink::WorkletGlobalScope>(ec))
    return wlgs->NodeIntegrationInWorker();

  auto* wgs = blink::DynamicTo<blink::WorkerGlobalScope>(ec);
  if (!wgs)
    return false;

  // Read the nodeIntegrationInWorker preference from the worker's settings,
  // which were copied from the initiating frame's WebPreferences at worker
  // creation time. This ensures that in-process child windows with different
  // webPreferences get the correct per-frame value rather than a process-wide
  // value.
  auto* worker_settings = wgs->GetWorkerSettings();
  return worker_settings && worker_settings->NodeIntegrationInWorker();
}

}  // namespace

void ElectronRendererClient::WorkerScriptReadyForEvaluationOnWorkerThread(
    v8::Local<v8::Context> context) {
  RendererClientBase::WorkerScriptReadyForEvaluationOnWorkerThread(context);

  auto* ec = blink::ExecutionContext::From(context);
  if (!WorkerHasNodeIntegration(ec))
    return;

  auto* current = WebWorkerObserver::GetCurrent();
  if (!current)
    current = WebWorkerObserver::Create(v8::Isolate::GetCurrent());
  current->WorkerScriptReadyForEvaluation(context);
}

void ElectronRendererClient::WillDestroyWorkerContextOnWorkerThread(
    v8::Local<v8::Context> context) {
  auto* ec = blink::ExecutionContext::From(context);
  if (WorkerHasNodeIntegration(ec)) {
    auto* current = WebWorkerObserver::GetCurrent();
    if (current)
      current->ContextWillDestroy(context);
  }

  // Call base class last: OOM callback deregistration must happen after
  // all other cleanup that might still trigger V8 heap operations.
  RendererClientBase::WillDestroyWorkerContextOnWorkerThread(context);
}

void ElectronRendererClient::SetUpWebAssemblyTrapHandler() {
#if ((BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC)) && \
     defined(ARCH_CPU_X86_64)) ||                                       \
    ((BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_MAC)) && defined(ARCH_CPU_ARM64))
  if (electron::fuses::IsWasmTrapHandlersEnabled()) {
    electron::SetUpWebAssemblyTrapHandler();
  }
#endif
}

node::Environment* ElectronRendererClient::GetEnvironment(
    content::RenderFrame* render_frame) const {
  auto iter = environments_.find(render_frame);
  return iter == environments_.end() ? nullptr
                                     : iter->second->environment.get();
}

}  // namespace electron
