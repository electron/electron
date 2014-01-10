// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "renderer/atom_renderer_client.h"

#include <algorithm>

#include "common/node_bindings.h"
#include "renderer/api/atom_renderer_bindings.h"
#include "renderer/atom_render_view_observer.h"

#include "common/v8/node_common.h"

namespace atom {

AtomRendererClient::AtomRendererClient()
    : node_bindings_(NodeBindings::Create(false)),
      atom_bindings_(new AtomRendererBindings) {
}

AtomRendererClient::~AtomRendererClient() {
}

void AtomRendererClient::RenderThreadStarted() {
  node_bindings_->Initialize();
  node_bindings_->PrepareMessageLoop();

  DCHECK(!global_env);

  // Create a default empty environment which would be used when we need to
  // run V8 code out of a window context (like running a uv callback).
  v8::HandleScope handle_scope(node_isolate);
  v8::Local<v8::Context> context = v8::Context::New(node_isolate);
  global_env = node::Environment::New(context);
}

void AtomRendererClient::RenderViewCreated(content::RenderView* render_view) {
  new AtomRenderViewObserver(render_view, this);
}

void AtomRendererClient::DidCreateScriptContext(WebKit::WebFrame* frame,
                                                v8::Handle<v8::Context> context,
                                                int extension_group,
                                                int world_id) {
  v8::Context::Scope scope(context);

  // Check the existance of process object to prevent duplicate initialization.
  if (context->Global()->Has(v8::String::New("process")))
    return;

  // Give the node loop a run to make sure everything is ready.
  node_bindings_->RunMessageLoop();

  // Setup node environment for each window.
  node::Environment* env = node_bindings_->CreateEnvironment(context);

  // Add atom-shell extended APIs.
  atom_bindings_->BindToFrame(frame);

  // Store the created environment.
  web_page_envs_.push_back(env);

  // Make uv loop being wrapped by window context.
  if (node_bindings_->uv_env() == NULL)
    node_bindings_->set_uv_env(env);
}

void AtomRendererClient::WillReleaseScriptContext(
    WebKit::WebFrame* frame,
    v8::Handle<v8::Context> context,
    int world_id) {
  node::Environment* env = node::Environment::GetCurrent(context);
  if (env == NULL) {
    LOG(ERROR) << "Encounter a non-node context when releasing script context";
    return;
  }

  // Clear the environment.
  web_page_envs_.erase(
      std::remove(web_page_envs_.begin(), web_page_envs_.end(), env),
      web_page_envs_.end());

  // Notice that we are not disposing the environment object here, because there
  // may still be pending uv operations in the uv loop, and when they got done
  // they would be needing the original environment.
  // So we are leaking the environment object here, just like Chrome leaking the
  // memory :) . Since it's only leaked when refreshing or unloading, so as long
  // as we make sure renderer process is restared then the memory would not be
  // leaked.
  // env->Dispose();

  // Wrap the uv loop with another environment.
  if (env == node_bindings_->uv_env()) {
    node::Environment* env = web_page_envs_.size() > 0 ? web_page_envs_[0] :
                                                         NULL;
    node_bindings_->set_uv_env(env);
  }
}

}  // namespace atom
