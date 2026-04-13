// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/renderer/web_worker_observer.h"

#include <string_view>
#include <utility>

#include "base/no_destructor.h"
#include "base/strings/strcat.h"
#include "base/threading/thread_local.h"
#include "gin/converter.h"
#include "shell/common/api/electron_bindings.h"
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
  active_context_count_++;

  if (environments_.empty()) {
    // First context on this thread - do full Node.js initialization.
    InitializeNewEnvironment(worker_context);
  } else {
    // Thread is being reused (AudioWorklet thread pooling). Share the
    // existing Node.js environment with the new context instead of
    // reinitializing, which would break existing contexts on this thread.
    ShareEnvironmentWithContext(worker_context);
  }
}

void WebWorkerObserver::InitializeNewEnvironment(
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

void WebWorkerObserver::ShareEnvironmentWithContext(
    v8::Local<v8::Context> worker_context) {
  v8::Context::Scope context_scope(worker_context);
  v8::Isolate* const isolate = v8::Isolate::GetCurrent();
  v8::MicrotasksScope microtasks_scope(
      worker_context, v8::MicrotasksScope::kDoNotRunMicrotasks);

  // Get the existing environment from the first context on this thread.
  DCHECK(!environments_.empty());
  node::Environment* env = environments_.begin()->get();

  // Initialize the V8 context for Node.js use.
  v8::Maybe<bool> initialized = node::InitializeContext(worker_context);
  CHECK(!initialized.IsNothing() && initialized.FromJust());

  // Assign the existing Node.js environment to this new context so that
  // node::Environment::GetCurrent(context) returns the shared environment.
  env->AssignToContext(worker_context, env->principal_realm(),
                       node::ContextInfo("electron_worker"));

  // Get process and require from the original context to make Node.js
  // APIs available in the new context.
  v8::Local<v8::Context> original_context = env->context();
  v8::Local<v8::Object> original_global = original_context->Global();
  v8::Local<v8::Object> new_global = worker_context->Global();

  v8::Local<v8::Value> process_value;
  CHECK(original_global
            ->Get(original_context, gin::StringToV8(isolate, "process"))
            .ToLocal(&process_value));

  v8::Local<v8::Value> require_value;
  CHECK(original_global
            ->Get(original_context, gin::StringToV8(isolate, "require"))
            .ToLocal(&require_value));

  // Set up 'global' as an alias for globalThis. Node.js bootstrapping normally
  // does this during LoadEnvironment, but we skip full bootstrap for shared
  // contexts.
  new_global
      ->Set(worker_context, gin::StringToV8(isolate, "global"), new_global)
      .Check();

  new_global
      ->Set(worker_context, gin::StringToV8(isolate, "process"), process_value)
      .Check();
  new_global
      ->Set(worker_context, gin::StringToV8(isolate, "require"), require_value)
      .Check();

  // Copy Buffer from the original context if it exists.
  v8::Local<v8::Value> buffer_value;
  if (original_global->Get(original_context, gin::StringToV8(isolate, "Buffer"))
          .ToLocal(&buffer_value) &&
      !buffer_value->IsUndefined()) {
    new_global
        ->Set(worker_context, gin::StringToV8(isolate, "Buffer"), buffer_value)
        .Check();
  }

  // Restore the Blink implementations of web APIs that Node.js may
  // have deleted. For first-context init this is done by the node_init script
  // but we can't run that for shared contexts (it calls internalBinding).
  // Instead, copy the blink-prefixed values set during first init.
  for (const std::string_view key :
       {"fetch", "Response", "FormData", "Request", "Headers", "EventSource"}) {
    // First, check if the new context has a working Blink version.
    v8::MaybeLocal<v8::Value> blink_value =
        new_global->Get(worker_context, gin::StringToV8(isolate, key));
    if (!blink_value.IsEmpty() && !blink_value.ToLocalChecked()->IsUndefined())
      continue;
    // If not, copy from the original context.
    std::string blink_key = base::StrCat({"blink", key});
    v8::Local<v8::Value> orig_value;
    if (original_global->Get(original_context, gin::StringToV8(isolate, key))
            .ToLocal(&orig_value) &&
        !orig_value->IsUndefined()) {
      new_global->Set(worker_context, gin::StringToV8(isolate, key), orig_value)
          .Check();
    }
  }
}

void WebWorkerObserver::ContextWillDestroy(v8::Local<v8::Context> context) {
  node::Environment* env = node::Environment::GetCurrent(context);
  if (!env)
    return;

  active_context_count_--;

  if (active_context_count_ == 0) {
    // Last context on this thread — full cleanup.
    {
      v8::Context::Scope context_scope(env->context());

      // Emit the "exit" event on the process object. We avoid using
      // gin_helper::EmitEvent here because it goes through
      // CallMethodWithArgs, which creates a node::CallbackScope. During
      // worker shutdown (PrepareForShutdownOnWorkerThread), the
      // CallbackScope destructor's InternalCallbackScope::Close() tries to
      // process ticks and microtask checkpoints, which can SEGV because the
      // worker context is being torn down by Blink.
      v8::Isolate* isolate = env->isolate();
      v8::HandleScope handle_scope(isolate);
      v8::Local<v8::Context> ctx = env->context();
      v8::Local<v8::Value> emit_v;
      if (env->process_object()
              ->Get(ctx, gin::StringToV8(isolate, "emit"))
              .ToLocal(&emit_v) &&
          emit_v->IsFunction()) {
        v8::Local<v8::Value> args[] = {gin::StringToV8(isolate, "exit")};
        v8::TryCatch try_catch(isolate);
        emit_v.As<v8::Function>()
            ->Call(ctx, env->process_object(), 1, args)
            .FromMaybe(v8::Local<v8::Value>());
      }
    }

    // Prevent UvRunOnce from using the environment after it's destroyed.
    node_bindings_->set_uv_env(nullptr);

    // Destroying the node environment will also run the uv loop.
    {
      util::ExplicitMicrotasksScope microtasks_scope(
          context->GetMicrotaskQueue());
      environments_.clear();
    }

    // ElectronBindings is tracking node environments.
    electron_bindings_->EnvironmentDestroyed(env);

    // Do NOT destroy the observer here. The worker thread may be reused
    // for another worklet context (e.g., AudioWorklet thread pooling or
    // PaintWorklet after page reload). Destroying the observer would
    // force a new NodeBindings to be created, but Node.js cannot be
    // fully reinitialized on the same thread (the allocator shim can
    // only be loaded once). Instead, keep the observer and its
    // NodeBindings alive so they can be reused. The environments_ set
    // is now empty, so WorkerScriptReadyForEvaluation will call
    // InitializeNewEnvironment on the next context.
  } else {
    // Other contexts still use the shared environment. Just unassign
    // this context from the environment if it's not the primary context
    // (the primary context must stay assigned because env->context()
    // references it, and UvRunOnce enters that context scope).
    if (context != env->context()) {
      env->UnassignFromContext(context);
    }
    // If the destroyed context IS the primary context, we leave the env
    // assigned to it. The env's PrincipalRealm holds a Global<Context>
    // reference that keeps the V8 context alive even though Blink has
    // torn down its side. This is safe because UvRunOnce only needs
    // the V8 context scope, not Blink-side objects.
  }
}

}  // namespace electron
