// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "atom/browser/devtools_web_contents_observer.h"

#include "atom/browser/api/atom_browser_bindings.h"
#include "atom/browser/atom_browser_main_parts.h"
#include "atom/browser/native_window.h"
#include "atom/common/api/api_messages.h"
#include "base/logging.h"
#include "content/public/browser/render_process_host.h"
#include "ipc/ipc_message_macros.h"

namespace atom {

DevToolsWebContentsObserver::DevToolsWebContentsObserver(
    NativeWindow* native_window,
    content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents),
      inspected_window_(native_window) {
  DCHECK(native_window);
}

DevToolsWebContentsObserver::~DevToolsWebContentsObserver() {
}

bool DevToolsWebContentsObserver::OnMessageReceived(
    const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(DevToolsWebContentsObserver, message)
    IPC_MESSAGE_HANDLER(AtomViewHostMsg_Message, OnRendererMessage)
    IPC_MESSAGE_HANDLER_DELAY_REPLY(AtomViewHostMsg_Message_Sync,
                                    OnRendererMessageSync)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()

  return handled;
}

void DevToolsWebContentsObserver::OnRendererMessage(
    const string16& channel,
    const base::ListValue& args) {
  AtomBrowserMainParts::Get()->atom_bindings()->OnRendererMessage(
      web_contents()->GetRenderProcessHost()->GetID(),
      web_contents()->GetRoutingID(),
      channel,
      args);
}

void DevToolsWebContentsObserver::OnRendererMessageSync(
    const string16& channel,
    const base::ListValue& args,
    IPC::Message* reply_msg) {
  AtomBrowserMainParts::Get()->atom_bindings()->OnRendererMessageSync(
      web_contents()->GetRenderProcessHost()->GetID(),
      web_contents()->GetRoutingID(),
      channel,
      args,
      web_contents(),
      reply_msg);
}

}  // namespace atom
