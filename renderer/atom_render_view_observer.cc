// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "renderer/atom_render_view_observer.h"

#include "common/node_bindings.h"
#include "renderer/atom_renderer_client.h"
#include "v8/include/v8.h"

namespace atom {

AtomRenderViewObserver::AtomRenderViewObserver(
    content::RenderView* render_view,
    AtomRendererClient* renderer_client)
    : content::RenderViewObserver(render_view),
      renderer_client_(renderer_client) {
}

AtomRenderViewObserver::~AtomRenderViewObserver() {
}

void AtomRenderViewObserver::DidClearWindowObject(WebKit::WebFrame* frame) {
  renderer_client_->node_bindings()->BindTo(frame);
}

}  // namespace atom
