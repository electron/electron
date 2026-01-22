// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/renderer/electron_renderer_client.h"

#include <algorithm>

#include "base/base_switches.h"
#include "base/command_line.h"
#include "base/debug/stack_trace.h"
#include "content/public/renderer/render_frame.h"
#include "net/http/http_request_headers.h"
#include "shell/common/api/electron_bindings.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/event_emitter_caller.h"
#include "shell/common/node_bindings.h"
#include "shell/common/node_includes.h"
#include "shell/common/node_util.h"
#include "shell/common/options_switches.h"
#include "shell/renderer/electron_render_frame_observer.h"
#include "shell/renderer/web_worker_observer.h"
#include "third_party/blink/public/common/web_preferences/web_preferences.h"
#include "third_party/blink/public/web/web_document.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"  // nogncheck
#include "third_party/blink/renderer/core/frame/web_local_frame_impl.h"  // nogncheck

#if BUILDFLAG(IS_LINUX) && (defined(ARCH_CPU_X86_64) || defined(ARCH_CPU_ARM64))
#define ENABLE_WEB_ASSEMBLY_TRAP_HANDLER_LINUX
#include "components/crash/core/app/crashpad.h"  // nogncheck
#include "content/public/common/content_switches.h"
#include "v8/include/v8-wasm-trap-handler-posix.h"
#endif

namespace electron {

ElectronRendererClient::ElectronRendererClient()
    : node_bindings_{NodeBindings::Create(
          NodeBindings::BrowserEnvironment::kRenderer)},
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
  // TODO(zcbenz): Do not create Node environment if node integration is not
  // enabled.

  // Only load Node.js if we are a main frame or a devtools extension
  // unless Node.js support has been explicitly enabled for subframes.
  if (!ShouldLoadPreload(isolate, renderer_context, render_frame))
    return;

  injected_frames_.insert(render_frame);

  if (!node_integration_initialized_) {
    node_integration_initialized_ = true;
    node_bindings_->Initialize(isolate, renderer_context);
    node_bindings_->PrepareEmbedThread();
  }

  // Setup node tracing controller.
  if (!node::tracing::TraceEventHelper::GetAgent()) {
    auto* tracing_agent = new node::tracing::Agent();
    node::tracing::TraceEventHelper::SetAgent(tracing_agent);
  }

  // Setup node environment for each window.
  v8::Maybe<bool> initialized = node::InitializeContext(renderer_context);
  CHECK(!initialized.IsNothing() && initialized.FromJust());

  // Before we load the node environment, let's tell blink to hold off on
  // loading the body of this frame.  We will undefer the load once the preload
  // script has finished.  This allows our preload script to run async (E.g.
  // with ESM) without the preload being in a race
  render_frame->GetWebFrame()->GetDocumentLoader()->SetDefersLoading(
      blink::LoaderFreezeMode::kStrict);

  std::shared_ptr<node::Environment> env = node_bindings_->CreateEnvironment(
      isolate, renderer_context, nullptr, 0,
      base::BindRepeating(&ElectronRendererClient::UndeferLoad,
                          base::Unretained(this), render_frame));

  // We need to use the Blink implementation of fetch in the renderer process
  // Node.js deletes the global fetch function when their fetch implementation
  // is disabled, so we need to save and re-add it after the Node.js environment
  // is loaded. See corresponding change in node/init.ts.
  v8::Local<v8::Object> global = renderer_context->Global();

  std::vector<std::string> keys = {"fetch",   "Response", "FormData",
                                   "Request", "Headers",  "EventSource"};
  for (const auto& key : keys) {
    v8::MaybeLocal<v8::Value> value =
        global->Get(renderer_context, gin::StringToV8(isolate, key));
    if (!value.IsEmpty()) {
      std::string blink_key = "blink" + key;
      global
          ->Set(renderer_context, gin::StringToV8(isolate, blink_key),
                value.ToLocalChecked())
          .Check();
    }
  }

  // If we have disabled the site instance overrides we should prevent loading
  // any non-context aware native module.
  env->options()->force_context_aware = true;

  // We do not want to crash the renderer process on unhandled rejections.
  env->options()->unhandled_rejections = "warn-with-error-code";

  environments_.insert(env);

  // Add Electron extended APIs.
  electron_bindings_->BindTo(env->isolate(), env->process_object());
  gin_helper::Dictionary process_dict(env->isolate(), env->process_object());
  BindProcess(env->isolate(), &process_dict, render_frame);

  // Load everything.
  node_bindings_->LoadEnvironment(env.get());

  if (node_bindings_->uv_env() == nullptr) {
    // Make uv loop being wrapped by window context.
    node_bindings_->set_uv_env(env.get());

    // Give the node loop a run to make sure everything is ready.
    node_bindings_->StartPolling();
  }
}

void ElectronRendererClient::WillReleaseScriptContext(
    v8::Isolate* const isolate,
    v8::Local<v8::Context> context,
    content::RenderFrame* render_frame) {
  if (injected_frames_.erase(render_frame) == 0)
    return;

  node::Environment* env = node::Environment::GetCurrent(context);
  const auto iter = std::ranges::find_if(
      environments_, [env](auto& item) { return env == item.get(); });
  if (iter == environments_.end())
    return;

  gin_helper::EmitEvent(isolate, env->process_object(), "exit");

  // The main frame may be replaced.
  if (env == node_bindings_->uv_env())
    node_bindings_->set_uv_env(nullptr);

  // Destroying the node environment will also run the uv loop.
  {
    util::ExplicitMicrotasksScope microtasks_scope(
        context->GetMicrotaskQueue());
    environments_.erase(iter);
  }

  // ElectronBindings is tracking node environments.
  electron_bindings_->EnvironmentDestroyed(env);
}

void ElectronRendererClient::WorkerScriptReadyForEvaluationOnWorkerThread(
    v8::Local<v8::Context> context) {
  // We do not create a Node.js environment in service or shared workers
  // owing to an inability to customize sandbox policies in these workers
  // given that they're run out-of-process.
  // Also avoid creating a Node.js environment for worklet global scope
  // created on the main thread.
  auto* ec = blink::ExecutionContext::From(context);
  if (ec->IsServiceWorkerGlobalScope() || ec->IsSharedWorkerGlobalScope() ||
      ec->IsMainThreadWorkletGlobalScope())
    return;

  // This won't be correct for in-process child windows with webPreferences
  // that have a different value for nodeIntegrationInWorker
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kNodeIntegrationInWorker)) {
    auto* current = WebWorkerObserver::GetCurrent();
    if (current)
      return;
    WebWorkerObserver::Create()->WorkerScriptReadyForEvaluation(context);
  }
}

void ElectronRendererClient::WillDestroyWorkerContextOnWorkerThread(
    v8::Local<v8::Context> context) {
  auto* ec = blink::ExecutionContext::From(context);
  if (ec->IsServiceWorkerGlobalScope() || ec->IsSharedWorkerGlobalScope() ||
      ec->IsMainThreadWorkletGlobalScope())
    return;

  // TODO(loc): Note that this will not be correct for in-process child windows
  // with webPreferences that have a different value for nodeIntegrationInWorker
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kNodeIntegrationInWorker)) {
    auto* current = WebWorkerObserver::GetCurrent();
    if (current)
      current->ContextWillDestroy(context);
  }
}

void ElectronRendererClient::SetUpWebAssemblyTrapHandler() {
// See CL:5372409 - copied from ShellContentRendererClient.
#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC)
  // Mac and Windows use the default implementation (where the default v8 trap
  // handler gets set up).
  ContentRendererClient::SetUpWebAssemblyTrapHandler();
  return;
#elif defined(ENABLE_WEB_ASSEMBLY_TRAP_HANDLER_LINUX)
  const bool crash_reporter_enabled =
      crash_reporter::GetHandlerSocket(nullptr, nullptr);

  if (crash_reporter_enabled) {
    // If either --enable-crash-reporter or --enable-crash-reporter-for-testing
    // is enabled it should take care of signal handling for us, use the default
    // implementation which doesn't register an additional handler.
    ContentRendererClient::SetUpWebAssemblyTrapHandler();
    return;
  }

  const bool use_v8_default_handler =
      base::CommandLine::ForCurrentProcess()->HasSwitch(
          ::switches::kDisableInProcessStackTraces);

  if (use_v8_default_handler) {
    // There is no signal handler yet, but it's okay if v8 registers one.
    v8::V8::EnableWebAssemblyTrapHandler(/*use_v8_signal_handler=*/true);
    return;
  }

  if (base::debug::SetStackDumpFirstChanceCallback(
          v8::TryHandleWebAssemblyTrapPosix)) {
    // Crashpad and Breakpad are disabled, but the in-process stack dump
    // handlers are enabled, so set the callback on the stack dump handlers.
    v8::V8::EnableWebAssemblyTrapHandler(/*use_v8_signal_handler=*/false);
    return;
  }

  // As the registration of the callback failed, we don't enable trap
  // handlers.
#endif  // defined(ENABLE_WEB_ASSEMBLY_TRAP_HANDLER_LINUX)
}

node::Environment* ElectronRendererClient::GetEnvironment(
    content::RenderFrame* render_frame) const {
  if (!injected_frames_.contains(render_frame))
    return nullptr;
  v8::HandleScope handle_scope(v8::Isolate::GetCurrent());
  auto context =
      GetContext(render_frame->GetWebFrame(), v8::Isolate::GetCurrent());
  node::Environment* env = node::Environment::GetCurrent(context);

  return std::ranges::contains(environments_, env,
                               [](auto const& item) { return item.get(); })
             ? env
             : nullptr;
}

}  // namespace electron
