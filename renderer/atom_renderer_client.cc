// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "renderer/atom_renderer_client.h"

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
  node_bindings_->CreateEnvironment(context);

  // Add atom-shell extended APIs.
  atom_bindings_->BindToFrame(frame);
}

void AtomRendererClient::WillReleaseScriptContext(WebKit::WebFrame* frame,
                                                  v8::Handle<v8::Context>,
                                                  int world_id) {
}

}  // namespace atom
