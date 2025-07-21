// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/renderer/web_worker_observer.h"

#include <string_view>
#include <utility>

#include "base/no_destructor.h"
#include "base/strings/strcat.h"
#include "base/threading/thread_local.h"
#include "shell/common/api/electron_bindings.h"
#include "shell/common/gin_helper/event_emitter_caller.h"
#include "shell/common/node_bindings.h"
#include "shell/common/node_includes.h"
#include "shell/common/node_util.h"

namespace electron {

namespace {

static base::NoDestructor<base::ThreadLocalOwnedPointer<WebWorkerObserver>>
    lazy_tls;

}  // namespace

// static
WebWorkerObserver* WebWorkerObserver::GetCurrent() {
  return lazy_tls->Get();
}

// static
WebWorkerObserver* WebWorkerObserver::Create() {
  auto obs = std::make_unique<WebWorkerObserver>();
  auto* obs_raw = obs.get();
  lazy_tls->Set(std::move(obs));
  return obs_raw;
}

WebWorkerObserver::WebWorkerObserver()
    : node_bindings_(
          NodeBindings::Create(NodeBindings::BrowserEnvironment::kWorker)),
      electron_bindings_(
          std::make_unique<ElectronBindings>(node_bindings_->uv_loop())) {}

WebWorkerObserver::~WebWorkerObserver() = default;

void WebWorkerObserver::WorkerScriptReadyForEvaluation(
    v8::Local<v8::Context> worker_context) {
  v8::Context::Scope context_scope(worker_context);
  v8::Isolate* const isolate = v8::Isolate::GetCurrent();
  v8::MicrotasksScope microtasks_scope(
      worker_context, v8::MicrotasksScope::kDoNotRunMicrotasks);

  // Start the embed thread.
  node_bindings_->PrepareEmbedThread();

  // Setup node tracing controller.
  if (!node::tracing::TraceEventHelper::GetAgent()) {
    auto* tracing_agent = new node::tracing::Agent();
    node::tracing::TraceEventHelper::SetAgent(tracing_agent);
  }

  // Setup node environment for each window.
  v8::Maybe<bool> initialized = node::InitializeContext(worker_context);
  CHECK(!initialized.IsNothing() && initialized.FromJust());
  std::shared_ptr<node::Environment> env =
      node_bindings_->CreateEnvironment(isolate, worker_context, nullptr);

  // We need to use the Blink implementation of fetch in web workers
  // Node.js deletes the global fetch function when their fetch implementation
  // is disabled, so we need to save and re-add it after the Node.js environment
  // is loaded. See corresponding change in node/init.ts.
  v8::Local<v8::Object> global = worker_context->Global();

  for (const std::string_view key :
       {"fetch", "Response", "FormData", "Request", "Headers", "EventSource"}) {
    v8::MaybeLocal<v8::Value> value =
        global->Get(worker_context, gin::StringToV8(isolate, key));
    if (!value.IsEmpty()) {
      std::string blink_key = base::StrCat({"blink", key});
      global
          ->Set(worker_context, gin::StringToV8(isolate, blink_key),
                value.ToLocalChecked())
          .Check();
    }
  }

  // We do not want to crash Web Workers on unhandled rejections.
  env->options()->unhandled_rejections = "warn-with-error-code";

  // Add Electron extended APIs.
  electron_bindings_->BindTo(env->isolate(), env->process_object());

  // Load everything.
  node_bindings_->LoadEnvironment(env.get());

  // Make uv loop being wrapped by window context.
  node_bindings_->set_uv_env(env.get());

  // Give the node loop a run to make sure everything is ready.
  node_bindings_->StartPolling();

  // Keep the environment alive until we free it in ContextWillDestroy()
  environments_.insert(std::move(env));
}

void WebWorkerObserver::ContextWillDestroy(v8::Local<v8::Context> context) {
  node::Environment* env = node::Environment::GetCurrent(context);
  if (env) {
    v8::Context::Scope context_scope(env->context());
    gin_helper::EmitEvent(env->isolate(), env->process_object(), "exit");
  }

  // Destroying the node environment will also run the uv loop.
  {
    util::ExplicitMicrotasksScope microtasks_scope(
        context->GetMicrotaskQueue());
    base::EraseIf(environments_,
                  [env](auto const& item) { return item.get() == env; });
  }

  // ElectronBindings is tracking node environments.
  electron_bindings_->EnvironmentDestroyed(env);

  if (lazy_tls->Get())
    lazy_tls->Set(nullptr);
}

}  // namespace electron
