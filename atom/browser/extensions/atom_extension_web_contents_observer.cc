// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/extensions/atom_extension_web_contents_observer.h"

#include <string>
#include "content/public/browser/browser_context.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "extensions/browser/extension_error.h"
#include "extensions/common/extension.h"
#include "extensions/common/extension_messages.h"
#include "extensions/common/extension_urls.h"
#include "extensions/renderer/console.h"

DEFINE_WEB_CONTENTS_USER_DATA_KEY(
    extensions::AtomExtensionWebContentsObserver);

namespace extensions {

AtomExtensionWebContentsObserver::AtomExtensionWebContentsObserver(
    content::WebContents* web_contents)
    : ExtensionWebContentsObserver(web_contents) { }

AtomExtensionWebContentsObserver::~AtomExtensionWebContentsObserver() {}
bool AtomExtensionWebContentsObserver::OnMessageReceived(
    const IPC::Message& message,
    content::RenderFrameHost* render_frame_host) {
  if (ExtensionWebContentsObserver::OnMessageReceived(message,
                                                      render_frame_host)) {
    return true;
  }

  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP_WITH_PARAM(AtomExtensionWebContentsObserver, message,
                                   render_frame_host)
    IPC_MESSAGE_HANDLER(ExtensionHostMsg_DetailedConsoleMessageAdded,
                        OnDetailedConsoleMessageAdded)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

void AtomExtensionWebContentsObserver::OnDetailedConsoleMessageAdded(
    content::RenderFrameHost* render_frame_host,
    const base::string16& message,
    const base::string16& source,
    const StackTrace& stack_trace,
    int32_t severity_level) {
  if (!IsSourceFromAnExtension(source))
    return;

  std::string extension_id = GetExtensionIdFromFrame(render_frame_host);
  if (extension_id.empty())
    extension_id = GURL(source).host();

  std::unique_ptr<ExtensionError> runtime_error(new RuntimeError(
          extension_id, browser_context()->IsOffTheRecord(), source, message,
          stack_trace, web_contents()->GetLastCommittedURL(),
          static_cast<logging::LogSeverity>(severity_level),
          render_frame_host->GetRoutingID(),
          render_frame_host->GetProcess()->GetID()));

  render_frame_host->AddMessageToConsole(
      static_cast<content::ConsoleMessageLevel>(severity_level),
      runtime_error->GetDebugString());
}

}  // namespace extensions
