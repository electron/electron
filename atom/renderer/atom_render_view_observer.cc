// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/renderer/atom_render_view_observer.h"

#include "atom/common/api/api_messages.h"
#include "content/public/renderer/render_view.h"
#include "ipc/ipc_message_macros.h"
#include "third_party/blink/public/web/web_view.h"

namespace atom {

AtomRenderViewObserver::AtomRenderViewObserver(content::RenderView* render_view)
    : content::RenderViewObserver(render_view) {}

AtomRenderViewObserver::~AtomRenderViewObserver() {}

bool AtomRenderViewObserver::OnMessageReceived(const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(AtomRenderViewObserver, message)
    IPC_MESSAGE_HANDLER(AtomViewMsg_Offscreen, OnOffscreen)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()

  return handled;
}

void AtomRenderViewObserver::OnDestruct() {
  delete this;
}

void AtomRenderViewObserver::OnOffscreen() {
  blink::WebView::SetUseExternalPopupMenus(false);
}

}  // namespace atom
