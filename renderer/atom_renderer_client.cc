// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "renderer/atom_renderer_client.h"

#include "renderer/atom_render_view_observer.h"

namespace atom {

AtomRendererClient::AtomRendererClient() {
}

AtomRendererClient::~AtomRendererClient() {
}

void AtomRendererClient::RenderViewCreated(content::RenderView* render_view) {
  new AtomRenderViewObserver(render_view);
}

}  // namespace atom
