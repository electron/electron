// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/renderer/atom_render_view_observer.h"

#include <string>
#include <vector>

#include "atom/common/api/api_messages.h"
#include "atom/common/options_switches.h"
#include "atom/renderer/api/atom_renderer_bindings.h"
#include "atom/renderer/atom_renderer_client.h"
#include "base/command_line.h"
#include "base/strings/string_number_conversions.h"
#include "content/public/renderer/render_view.h"
#include "ipc/ipc_message_macros.h"
#include "third_party/WebKit/public/web/WebDraggableRegion.h"
#include "third_party/WebKit/public/web/WebDocument.h"
#include "third_party/WebKit/public/web/WebFrame.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"
#include "third_party/WebKit/public/web/WebView.h"

#include "atom/common/node_includes.h"

using blink::WebFrame;

namespace atom {

AtomRenderViewObserver::AtomRenderViewObserver(
    content::RenderView* render_view,
    AtomRendererClient* renderer_client)
    : content::RenderViewObserver(render_view),
      renderer_client_(renderer_client) {
}

AtomRenderViewObserver::~AtomRenderViewObserver() {
}

void AtomRenderViewObserver::DidCreateDocumentElement(
    blink::WebLocalFrame* frame) {
  // Read --zoom-factor from command line.
  std::string zoom_factor_str = CommandLine::ForCurrentProcess()->
      GetSwitchValueASCII(switches::kZoomFactor);;
  if (zoom_factor_str.empty())
    return;
  double zoom_factor;
  if (!base::StringToDouble(zoom_factor_str, &zoom_factor))
    return;
  double zoom_level = blink::WebView::zoomFactorToZoomLevel(zoom_factor);
  frame->view()->setZoomLevel(zoom_level);
}

void AtomRenderViewObserver::DraggableRegionsChanged(blink::WebFrame* frame) {
  blink::WebVector<blink::WebDraggableRegion> webregions =
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

void AtomRenderViewObserver::OnBrowserMessage(const base::string16& channel,
                                              const base::ListValue& args) {
  renderer_client_->atom_bindings()->OnBrowserMessage(
      render_view(), channel, args);
}

}  // namespace atom
