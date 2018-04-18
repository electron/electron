// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/renderer/web_worker_observer.h"

#include "atom/common/api/atom_bindings.h"
#include "atom/common/api/event_emitter_caller.h"
#include "atom/common/asar/asar_util.h"
#include "atom/common/node_bindings.h"
#include "base/lazy_instance.h"
#include "base/threading/thread_local.h"

#include "atom/common/node_includes.h"

namespace atom {

namespace {

static base::LazyInstance<
    base::ThreadLocalPointer<WebWorkerObserver>>::DestructorAtExit lazy_tls =
    LAZY_INSTANCE_INITIALIZER;

}  // namespace

// static
WebWorkerObserver* WebWorkerObserver::GetCurrent() {
  WebWorkerObserver* self = lazy_tls.Pointer()->Get();
  return self ? self : new WebWorkerObserver;
}

WebWorkerObserver::WebWorkerObserver()
    : node_bindings_(NodeBindings::Create(NodeBindings::WORKER)),
      atom_bindings_(new AtomBindings(node_bindings_->uv_loop())) {
  lazy_tls.Pointer()->Set(this);
}

WebWorkerObserver::~WebWorkerObserver() {
  lazy_tls.Pointer()->Set(nullptr);
  node::FreeEnvironment(node_bindings_->uv_env());
  asar::ClearArchives();
}

void WebWorkerObserver::ContextCreated(v8::Local<v8::Context> context) {
  v8::Context::Scope context_scope(context);

  // Start the embed thread.
  node_bindings_->PrepareMessageLoop();

  // Setup node environment for each window.
  node::Environment* env = node_bindings_->CreateEnvironment(context);

  // Add Electron extended APIs.
  atom_bindings_->BindTo(env->isolate(), env->process_object());

  // Load everything.
  node_bindings_->LoadEnvironment(env);

  // Make uv loop being wrapped by window context.
  node_bindings_->set_uv_env(env);

  // Give the node loop a run to make sure everything is ready.
  node_bindings_->RunMessageLoop();
}

void WebWorkerObserver::ContextWillDestroy(v8::Local<v8::Context> context) {
  node::Environment* env = node::Environment::GetCurrent(context);
  if (env)
    mate::EmitEvent(env->isolate(), env->process_object(), "exit");

  delete this;
}

}  // namespace atom
