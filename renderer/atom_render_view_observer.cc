// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "renderer/atom_render_view_observer.h"

#include "common/api/api_messages.h"
#include "ipc/ipc_message_macros.h"
#include "renderer/api/atom_renderer_bindings.h"
#include "renderer/atom_renderer_client.h"
#include "third_party/WebKit/public/web/WebDraggableRegion.h"
#include "third_party/WebKit/public/web/WebDocument.h"
#include "third_party/WebKit/public/web/WebFrame.h"

#include "common/v8/node_common.h"

using WebKit::WebFrame;

namespace atom {

AtomRenderViewObserver::AtomRenderViewObserver(
    content::RenderView* render_view,
    AtomRendererClient* renderer_client)
    : content::RenderViewObserver(render_view),
      renderer_client_(renderer_client) {
}

AtomRenderViewObserver::~AtomRenderViewObserver() {
}

void AtomRenderViewObserver::DraggableRegionsChanged(WebKit::WebFrame* frame) {
  WebKit::WebVector<WebKit::WebDraggableRegion> webregions =
      frame->document().draggableRegions();
  std::vector<DraggableRegion> regions;
  for (size_t i = 0; i < webregions.size(); ++i) {
    DraggableRegion region;
    region.bounds = webregions[i].bounds;
    region.draggable = webregions[i].draggable;
    regions.push_back(region);
  }
  Send(new AtomViewHostMsg_UpdateDraggableRegions(routing_id(), regions));
}

bool AtomRenderViewObserver::OnMessageReceived(const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(AtomRenderViewObserver, message)
    IPC_MESSAGE_HANDLER(AtomViewMsg_Message, OnBrowserMessage)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()

  return handled;
}

void AtomRenderViewObserver::OnBrowserMessage(const string16& channel,
                                              const base::ListValue& args) {
  renderer_client_->atom_bindings()->OnBrowserMessage(
      render_view(), channel, args);
}

}  // namespace atom
