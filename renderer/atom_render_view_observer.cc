// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "renderer/atom_render_view_observer.h"

#include <algorithm>
#include <vector>

#include "common/api/atom_bindings.h"
#include "common/node_bindings.h"
#include "renderer/atom_renderer_client.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebFrame.h"
#include "v8/include/v8.h"

using WebKit::WebFrame;

namespace webkit_atom {
extern void SetNodeContext(v8::Persistent<v8::Context> context);
extern void SetEnterFirstWindowContext(bool (*func)());
extern void SetIsValidWindowContext(bool (*func)(v8::Handle<v8::Context>));
}

namespace atom {

namespace {

std::vector<WebFrame*>& web_frames() {
  CR_DEFINE_STATIC_LOCAL(std::vector<WebFrame*>, frames, ());
  return frames;
}

bool EnterFirstWindowContext() {
  if (web_frames().size() == 0)
    return false;

  DCHECK(!web_frames()[0]->mainWorldScriptContext().IsEmpty());
  web_frames()[0]->mainWorldScriptContext()->Enter();
  return true;
}

bool IsValidWindowContext(v8::Handle<v8::Context> context) {
  size_t size = web_frames().size();
  for (size_t i = 0; i < size; ++i)
    if (web_frames()[i]->mainWorldScriptContext() == context)
      return true;

  return false;
}

}  // namespace

AtomRenderViewObserver::AtomRenderViewObserver(
    content::RenderView* render_view,
    AtomRendererClient* renderer_client)
    : content::RenderViewObserver(render_view),
      renderer_client_(renderer_client) {
  // Interact with dirty workarounds of extra node context in WebKit.
  webkit_atom::SetEnterFirstWindowContext(EnterFirstWindowContext);
  webkit_atom::SetIsValidWindowContext(IsValidWindowContext);
}

AtomRenderViewObserver::~AtomRenderViewObserver() {
}

void AtomRenderViewObserver::DidClearWindowObject(WebFrame* frame) {
  // Remember the web frame.
  web_frames().push_back(frame);

  renderer_client_->node_bindings()->BindTo(frame);
  renderer_client_->atom_bindings()->BindToFrame(frame);
}

void AtomRenderViewObserver::FrameWillClose(WebFrame* frame) {
  std::vector<WebFrame*>& vec = web_frames();
  vec.erase(std::remove(vec.begin(), vec.end(), frame), vec.end());
}

}  // namespace atom
