// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "renderer/atom_renderer_client.h"

#include "common/node_bindings.h"
#include "renderer/atom_render_view_observer.h"
#include "vendor/node/src/node_internals.h"

namespace webkit_atom {
extern void SetNodeContext(v8::Persistent<v8::Context> context);
}

namespace atom {

AtomRendererClient::AtomRendererClient()
    : node_bindings_(NodeBindings::Create(false)) {
}

AtomRendererClient::~AtomRendererClient() {
}

void AtomRendererClient::RenderThreadStarted() {
  node_bindings_->Initialize();

  // Interact with dirty workarounds of extra node context in WebKit.
  webkit_atom::SetNodeContext(node::g_context);

  node_bindings_->Load();
  node_bindings_->PrepareMessageLoop();
  node_bindings_->RunMessageLoop();
}

void AtomRendererClient::RenderViewCreated(content::RenderView* render_view) {
  new AtomRenderViewObserver(render_view, this);
}

}  // namespace atom
