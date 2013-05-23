// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "renderer/atom_render_view_observer.h"

#include <algorithm>
#include <vector>

#include "common/api/api_messages.h"
#include "common/node_bindings.h"
#include "ipc/ipc_message_macros.h"
#include "renderer/api/atom_renderer_bindings.h"
#include "renderer/atom_renderer_client.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebFrame.h"
#include "v8/include/v8.h"

using WebKit::WebFrame;

namespace webkit {
extern void SetGetFirstWindowContext(v8::Handle<v8::Context> (*)());
extern void SetIsValidWindowContext(bool (*)(v8::Handle<v8::Context>));
}

namespace atom {

namespace {

std::vector<WebFrame*>& web_frames() {
  CR_DEFINE_STATIC_LOCAL(std::vector<WebFrame*>, frames, ());
  return frames;
}

v8::Handle<v8::Context> GetFirstWindowContext() {
  if (web_frames().size() == 0)
    return v8::Handle<v8::Context>();

  return web_frames()[0]->mainWorldScriptContext();
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
      atom_bindings_(new AtomRendererBindings(render_view)),
      renderer_client_(renderer_client) {
  // Interact with dirty workarounds of extra node context in WebKit.
  webkit::SetGetFirstWindowContext(GetFirstWindowContext);
  webkit::SetIsValidWindowContext(IsValidWindowContext);
}

AtomRenderViewObserver::~AtomRenderViewObserver() {
}

void AtomRenderViewObserver::DidClearWindowObject(WebFrame* frame) {
  // Remember the web frame.
  web_frames().push_back(frame);

  renderer_client_->node_bindings()->BindTo(frame);
  atom_bindings()->BindToFrame(frame);
}

void AtomRenderViewObserver::FrameWillClose(WebFrame* frame) {
  std::vector<WebFrame*>& vec = web_frames();
  vec.erase(std::remove(vec.begin(), vec.end(), frame), vec.end());
}

bool AtomRenderViewObserver::OnMessageReceived(const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(AtomRenderViewObserver, message)
    IPC_MESSAGE_HANDLER(AtomViewMsg_Message, OnBrowserMessage)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()

  return handled;
}

void AtomRenderViewObserver::OnBrowserMessage(const std::string& channel,
                                              const base::ListValue& args) {
  atom_bindings()->OnBrowserMessage(channel, args);
}

}  // namespace atom
