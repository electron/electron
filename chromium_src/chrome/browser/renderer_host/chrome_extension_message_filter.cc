// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/renderer_host/chrome_extension_message_filter.h"

#include <stdint.h>

#include "atom/browser/extensions/api/messaging/message_service.h"
#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/chrome_notification_types.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/render_process_host.h"
#include "extensions/browser/extension_system.h"
#include "extensions/browser/info_map.h"
#include "extensions/common/api/messaging/message.h"
#include "extensions/common/extension_messages.h"
#include "extensions/common/file_util.h"
#include "extensions/common/manifest_handlers/default_locale_handler.h"
#include "extensions/common/message_bundle.h"

using content::BrowserThread;

namespace {

const uint32_t kFilteredMessageClasses[] = {
    ChromeExtensionMsgStart, ExtensionMsgStart,
};

}

ChromeExtensionMessageFilter::ChromeExtensionMessageFilter(
    int render_process_id,
    content::BrowserContext* browser_context)
    : BrowserMessageFilter(kFilteredMessageClasses,
                           arraysize(kFilteredMessageClasses)),
      render_process_id_(render_process_id),
      browser_context_(browser_context),
      extension_info_map_(
          extensions::ExtensionSystem::Get(browser_context)->info_map()) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  notification_registrar_.Add(this,
                              chrome::NOTIFICATION_PROFILE_DESTROYED,
                              content::Source<content::BrowserContext>(browser_context));
}

ChromeExtensionMessageFilter::~ChromeExtensionMessageFilter() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
}

bool ChromeExtensionMessageFilter::OnMessageReceived(
    const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(ChromeExtensionMessageFilter, message)
    IPC_MESSAGE_HANDLER(ExtensionHostMsg_OpenChannelToExtension,
                        OnOpenChannelToExtension)
    IPC_MESSAGE_HANDLER(ExtensionHostMsg_OpenChannelToTab, OnOpenChannelToTab)
    IPC_MESSAGE_HANDLER(ExtensionHostMsg_OpenChannelToNativeApp,
                        OnOpenChannelToNativeApp)
    IPC_MESSAGE_HANDLER(ExtensionHostMsg_OpenMessagePort, OnOpenMessagePort)
    IPC_MESSAGE_HANDLER(ExtensionHostMsg_CloseMessagePort, OnCloseMessagePort)
    IPC_MESSAGE_HANDLER(ExtensionHostMsg_PostMessage, OnPostMessage)
    IPC_MESSAGE_HANDLER_DELAY_REPLY(ExtensionHostMsg_GetMessageBundle,
                                    OnGetExtMessageBundle)
    IPC_MESSAGE_HANDLER(ExtensionHostMsg_AddAPIActionToActivityLog,
                        OnAddAPIActionToExtensionActivityLog);
    IPC_MESSAGE_HANDLER(ExtensionHostMsg_AddDOMActionToActivityLog,
                        OnAddDOMActionToExtensionActivityLog);
    IPC_MESSAGE_HANDLER(ExtensionHostMsg_AddEventToActivityLog,
                        OnAddEventToExtensionActivityLog);
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()

  return handled;
}

void ChromeExtensionMessageFilter::OverrideThreadForMessage(
    const IPC::Message& message, BrowserThread::ID* thread) {
  switch (message.type()) {
    case ExtensionHostMsg_OpenMessagePort::ID:
    case ExtensionHostMsg_CloseMessagePort::ID:
    case ExtensionHostMsg_PostMessage::ID:
    case ExtensionHostMsg_AddAPIActionToActivityLog::ID:
    case ExtensionHostMsg_AddDOMActionToActivityLog::ID:
    case ExtensionHostMsg_AddEventToActivityLog::ID:
      *thread = BrowserThread::UI;
    default:
      break;
  }
}

void ChromeExtensionMessageFilter::OnDestruct() const {
  if (BrowserThread::CurrentlyOn(BrowserThread::UI)) {
    delete this;
  } else {
    BrowserThread::DeleteSoon(BrowserThread::UI, FROM_HERE, this);
  }
}

void ChromeExtensionMessageFilter::OnOpenChannelToExtension(
    int routing_id,
    const ExtensionMsg_ExternalConnectionInfo& info,
    const std::string& channel_name,
    bool include_tls_channel_id,
    int* port_id) {
  int port2_id;
  extensions::MessageService::AllocatePortIdPair(port_id, &port2_id);

  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::Bind(
          &ChromeExtensionMessageFilter::OpenChannelToExtensionOnUIThread,
          this, render_process_id_, routing_id, port2_id, info,
          channel_name, include_tls_channel_id));
}

void ChromeExtensionMessageFilter::OpenChannelToExtensionOnUIThread(
    int source_process_id, int source_routing_id,
    int receiver_port_id,
    const ExtensionMsg_ExternalConnectionInfo& info,
    const std::string& channel_name,
    bool include_tls_channel_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (browser_context_) {
    extensions::MessageService::Get(browser_context_)
        ->OpenChannelToExtension(source_process_id,
                                 source_routing_id,
                                 receiver_port_id,
                                 info.source_id,
                                 info.target_id,
                                 info.source_url,
                                 channel_name,
                                 include_tls_channel_id);
  }
}

void ChromeExtensionMessageFilter::OnOpenChannelToNativeApp(
    int routing_id,
    const std::string& native_app_name,
    int* port_id) {
  int port2_id;
  extensions::MessageService::AllocatePortIdPair(port_id, &port2_id);

  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::Bind(
          &ChromeExtensionMessageFilter::OpenChannelToNativeAppOnUIThread,
          this, routing_id, port2_id, native_app_name));
}

void ChromeExtensionMessageFilter::OpenChannelToNativeAppOnUIThread(
    int source_routing_id,
    int receiver_port_id,
    const std::string& native_app_name) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (browser_context_) {
    extensions::MessageService::Get(browser_context_)
        ->OpenChannelToNativeApp(render_process_id_,
                                 source_routing_id,
                                 receiver_port_id,
                                 native_app_name);
  }
}

void ChromeExtensionMessageFilter::OnOpenChannelToTab(
    int routing_id,
    const ExtensionMsg_TabTargetConnectionInfo& info,
    const std::string& extension_id,
    const std::string& channel_name,
    int* port_id) {
  int port2_id;
  extensions::MessageService::AllocatePortIdPair(port_id, &port2_id);

  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::Bind(&ChromeExtensionMessageFilter::OpenChannelToTabOnUIThread,
                 this, render_process_id_, routing_id, port2_id, info,
                 extension_id, channel_name));
}

void ChromeExtensionMessageFilter::OpenChannelToTabOnUIThread(
    int source_process_id,
    int source_routing_id,
    int receiver_port_id,
    const ExtensionMsg_TabTargetConnectionInfo& info,
    const std::string& extension_id,
    const std::string& channel_name) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (browser_context_) {
    extensions::MessageService::Get(browser_context_)
        ->OpenChannelToTab(source_process_id,
                           source_routing_id,
                           receiver_port_id,
                           info.tab_id,
                           info.frame_id,
                           extension_id,
                           channel_name);
  }
}

void ChromeExtensionMessageFilter::OnOpenMessagePort(int routing_id,
                                                     int port_id) {
  if (!browser_context_)
    return;

  extensions::MessageService::Get(browser_context_)->OpenPort(
      port_id, render_process_id_, routing_id);
}

void ChromeExtensionMessageFilter::OnCloseMessagePort(int routing_id,
                                                      int port_id,
                                                      bool force_close) {
  if (!browser_context_)
    return;

  extensions::MessageService::Get(browser_context_)->ClosePort(
      port_id, render_process_id_, routing_id, force_close);
}

void ChromeExtensionMessageFilter::OnPostMessage(
    int port_id,
    const extensions::Message& message) {
  if (!browser_context_)
    return;

  // this method is called on the UI thread, but there is a race condition
  // with OnOpenChannelToTab and this ensures that PostMessage runs after
  // OpenChannelToTab
  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::Bind(&ChromeExtensionMessageFilter::OnPostMessageOnUIThread,
                 this, port_id, message));
}

void ChromeExtensionMessageFilter::OnPostMessageOnUIThread(
    int port_id,
    const extensions::Message& message) {
  extensions::MessageService::Get(browser_context_)->PostMessage(port_id, message);
}

void ChromeExtensionMessageFilter::OnGetExtMessageBundle(
    const std::string& extension_id, IPC::Message* reply_msg) {
  BrowserThread::PostBlockingPoolTask(
      FROM_HERE,
      base::Bind(
          &ChromeExtensionMessageFilter::OnGetExtMessageBundleOnBlockingPool,
          this, extension_id, reply_msg));
}

void ChromeExtensionMessageFilter::OnGetExtMessageBundleOnBlockingPool(
    const std::string& extension_id,
    IPC::Message* reply_msg) {
  DCHECK(BrowserThread::GetBlockingPool()->RunsTasksOnCurrentThread());

  const extensions::ExtensionSet& extension_set =
      extension_info_map_->extensions();

  std::unique_ptr<extensions::MessageBundle::SubstitutionMap> dictionary_map(
      extensions::file_util::LoadMessageBundleSubstitutionMapWithImports(
          extension_id, extension_set));

  ExtensionHostMsg_GetMessageBundle::WriteReplyParams(reply_msg,
                                                      *dictionary_map);
  Send(reply_msg);
}

void ChromeExtensionMessageFilter::OnAddAPIActionToExtensionActivityLog(
    const std::string& extension_id,
    const ExtensionHostMsg_APIActionOrEvent_Params& params) {
  VLOG(1) << "extension action " << params.api_call << " " << params.arguments;
}

void ChromeExtensionMessageFilter::OnAddDOMActionToExtensionActivityLog(
    const std::string& extension_id,
    const ExtensionHostMsg_DOMAction_Params& params) {
  VLOG(1) << "dom action " << params.api_call << " " << params.arguments;
}

void ChromeExtensionMessageFilter::OnAddEventToExtensionActivityLog(
    const std::string& extension_id,
    const ExtensionHostMsg_APIActionOrEvent_Params& params) {
  VLOG(1) << "extension event " << params.api_call << " " << params.arguments;
}

void ChromeExtensionMessageFilter::Observe(
    int type,
    const content::NotificationSource& source,
    const content::NotificationDetails& details) {
  DCHECK_EQ(chrome::NOTIFICATION_PROFILE_DESTROYED, type);
  browser_context_ = NULL;
}
