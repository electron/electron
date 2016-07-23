// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "atom/browser/extensions/api/messaging/message_service.h"

#include <stdint.h>
#include <limits>
#include <utility>

#include "atom/browser/extensions/api/messaging/extension_message_port.h"
#include "extensions/browser/extension_web_contents_observer.h"
#include "atom/browser/extensions/atom_extensions_browser_client.h"
#include "atom/browser/extensions/atom_extension_system.h"
#include "atom/browser/extensions/tab_helper.h"
#include "base/atomic_sequence_num.h"
#include "base/bind.h"
#include "base/callback.h"
#include "base/json/json_writer.h"
#include "base/lazy_instance.h"
#include "base/macros.h"
#include "base/metrics/histogram.h"
#include "components/prefs/pref_service.h"
#include "base/stl_util.h"
#include "build/build_config.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/site_instance.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/child_process_host.h"
#include "extensions/browser/event_router.h"
#include "extensions/browser/extension_api_frame_id_map.h"
#include "extensions/browser/extension_host.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_system.h"
#include "extensions/browser/extension_util.h"
#include "extensions/browser/lazy_background_task_queue.h"
#include "extensions/browser/process_manager.h"
#include "extensions/common/extension.h"
#include "extensions/common/manifest.h"
#include "extensions/common/manifest_constants.h"
#include "extensions/common/manifest_handlers/background_info.h"
#include "extensions/common/manifest_handlers/externally_connectable.h"
#include "extensions/common/manifest_handlers/incognito_info.h"
#include "extensions/common/permissions/permissions_data.h"
#include "net/base/completion_callback.h"
#include "url/gurl.h"

using content::BrowserContext;
using content::BrowserThread;
using content::SiteInstance;
using content::WebContents;

// Since we have 2 ports for every channel, we just index channels by half the
// port ID.
#define GET_CHANNEL_ID(port_id) ((port_id) / 2)
#define GET_CHANNEL_OPENER_ID(channel_id) ((channel_id) * 2)
#define GET_CHANNEL_RECEIVERS_ID(channel_id) ((channel_id) * 2 + 1)

// Port1 is always even, port2 is always odd.
#define IS_OPENER_PORT_ID(port_id) (((port_id) & 1) == 0)

// Change even to odd and vice versa, to get the other side of a given channel.
#define GET_OPPOSITE_PORT_ID(source_port_id) ((source_port_id) ^ 1)

namespace {

content::WebContents* GetWebContentsByFrameID(int render_process_id,
                                              int render_frame_id) {
  content::RenderFrameHost* render_frame_host =
      content::RenderFrameHost::FromID(render_process_id, render_frame_id);
  if (!render_frame_host)
    return NULL;
  return content::WebContents::FromRenderFrameHost(render_frame_host);
}

}  // namespace

namespace extensions {

const char kReceivingEndDoesntExistError[] =
    "Could not establish connection. Receiving end does not exist.";
const char kMissingPermissionError[] =
    "Access to native messaging requires nativeMessaging permission.";
const char kProhibitedByPoliciesError[] =
    "Access to the native messaging host was disabled by the system "
    "administrator.";

struct MessageService::MessageChannel {
  std::unique_ptr<MessagePort> opener;
  std::unique_ptr<MessagePort> receiver;
};

struct MessageService::OpenChannelParams {
  int source_process_id;
  int source_routing_id;
  std::unique_ptr<base::DictionaryValue> source_tab;
  int source_frame_id;
  std::unique_ptr<MessagePort> receiver;
  int receiver_port_id;
  std::string source_extension_id;
  std::string target_extension_id;
  GURL source_url;
  std::string channel_name;
  bool include_tls_channel_id;
  std::string tls_channel_id;
  bool include_guest_process_info;

  // Takes ownership of receiver.
  OpenChannelParams(int source_process_id,
                    int source_routing_id,
                    std::unique_ptr<base::DictionaryValue> source_tab,
                    int source_frame_id,
                    MessagePort* receiver,
                    int receiver_port_id,
                    const std::string& source_extension_id,
                    const std::string& target_extension_id,
                    const GURL& source_url,
                    const std::string& channel_name,
                    bool include_tls_channel_id,
                    bool include_guest_process_info)
      : source_process_id(source_process_id),
        source_routing_id(source_routing_id),
        source_frame_id(source_frame_id),
        receiver(receiver),
        receiver_port_id(receiver_port_id),
        source_extension_id(source_extension_id),
        target_extension_id(target_extension_id),
        source_url(source_url),
        channel_name(channel_name),
        include_tls_channel_id(include_tls_channel_id),
        include_guest_process_info(include_guest_process_info) {
    if (source_tab)
      this->source_tab = std::move(source_tab);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(OpenChannelParams);
};

namespace {

static base::StaticAtomicSequenceNumber g_next_channel_id;

static content::RenderProcessHost* GetExtensionProcess(
    BrowserContext* context,
    const std::string& extension_id) {
  scoped_refptr<SiteInstance> site_instance =
      ProcessManager::Get(context)->GetSiteInstanceForURL(
          Extension::GetBaseURLFromExtensionId(extension_id));
  return site_instance->HasProcess() ? site_instance->GetProcess() : NULL;
}

}  // namespace

void MessageService::MessagePort::RemoveCommonFrames(const MessagePort& port) {}

bool MessageService::MessagePort::HasFrame(
    content::RenderFrameHost* rfh) const {
  return false;
}

// static
void MessageService::AllocatePortIdPair(int* port1, int* port2) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  unsigned channel_id = static_cast<unsigned>(g_next_channel_id.GetNext()) %
                        (std::numeric_limits<int32_t>::max() / 2);
  unsigned port1_id = channel_id * 2;
  unsigned port2_id = channel_id * 2 + 1;

  // Sanity checks to make sure our channel<->port converters are correct.
  DCHECK(IS_OPENER_PORT_ID(port1_id));
  DCHECK_EQ(GET_OPPOSITE_PORT_ID(port1_id), port2_id);
  DCHECK_EQ(GET_OPPOSITE_PORT_ID(port2_id), port1_id);
  DCHECK_EQ(GET_CHANNEL_ID(port1_id), GET_CHANNEL_ID(port2_id));
  DCHECK_EQ(GET_CHANNEL_ID(port1_id), channel_id);
  DCHECK_EQ(GET_CHANNEL_OPENER_ID(channel_id), port1_id);
  DCHECK_EQ(GET_CHANNEL_RECEIVERS_ID(channel_id), port2_id);

  *port1 = port1_id;
  *port2 = port2_id;
}

MessageService::MessageService(BrowserContext* context)
    : lazy_background_task_queue_(
          LazyBackgroundTaskQueue::Get(context)),
      weak_factory_(this) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
}

MessageService::~MessageService() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  STLDeleteContainerPairSecondPointers(channels_.begin(), channels_.end());
  channels_.clear();
}

static base::LazyInstance<BrowserContextKeyedAPIFactory<MessageService> >
    g_factory = LAZY_INSTANCE_INITIALIZER;

// static
BrowserContextKeyedAPIFactory<MessageService>*
MessageService::GetFactoryInstance() {
  return g_factory.Pointer();
}

// static
MessageService* MessageService::Get(BrowserContext* context) {
  return BrowserContextKeyedAPIFactory<MessageService>::Get(context);
}

void MessageService::OpenChannelToExtension(
    int source_process_id, int source_routing_id, int receiver_port_id,
    const std::string& source_extension_id,
    const std::string& target_extension_id,
    const GURL& source_url,
    const std::string& channel_name,
    bool include_tls_channel_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  content::RenderFrameHost* source =
      content::RenderFrameHost::FromID(source_process_id, source_routing_id);
  if (!source)
    return;

  BrowserContext* context = source->GetProcess()->GetBrowserContext();

  ExtensionRegistry* registry = ExtensionRegistry::Get(context);
  const Extension* target_extension =
      registry->enabled_extensions().GetByID(target_extension_id);
  if (!target_extension) {
    DispatchOnDisconnect(
        source, receiver_port_id, kReceivingEndDoesntExistError);
    return;
  }

  bool is_web_connection = false;

  if (source_extension_id != target_extension_id) {
    // It's an external connection. Check the externally_connectable manifest
    // key if it's present. If it's not, we allow connection from any extension
    // but not webpages.
    ExternallyConnectableInfo* externally_connectable =
        static_cast<ExternallyConnectableInfo*>(
            target_extension->GetManifestData(
                manifest_keys::kExternallyConnectable));
    bool is_externally_connectable = false;

    if (externally_connectable) {
      if (source_extension_id.empty()) {
        // No source extension ID so the source was a web page. Check that the
        // URL matches.
        is_web_connection = true;
        is_externally_connectable =
            externally_connectable->matches.MatchesURL(source_url);
        // Only include the TLS channel ID for externally connected web pages.
        include_tls_channel_id &=
            is_externally_connectable &&
            externally_connectable->accepts_tls_channel_id;
      } else {
        // Source extension ID so the source was an extension. Check that the
        // extension matches.
        is_externally_connectable =
            externally_connectable->IdCanConnect(source_extension_id);
      }
    } else {
      // Default behaviour. Any extension, no webpages.
      is_externally_connectable = !source_extension_id.empty();
    }

    if (!is_externally_connectable) {
      // Important: use kReceivingEndDoesntExistError here so that we don't
      // leak information about this extension to callers. This way it's
      // indistinguishable from the extension just not existing.
      DispatchOnDisconnect(
          source, receiver_port_id, kReceivingEndDoesntExistError);
      return;
    }
  }

  WebContents* source_contents = GetWebContentsByFrameID(
      source_process_id, source_routing_id);

  bool include_guest_process_info = false;

  // Include info about the opener's tab (if it was a tab).
  std::unique_ptr<base::DictionaryValue> source_tab;
  int source_frame_id = -1;
  if (source_contents && TabHelper::IdForTab(source_contents) >= 0) {
    // Only the tab id is useful to platform apps for internal use. The
    // unnecessary bits will be stripped out in
    // MessagingBindings::DispatchOnConnect().
    source_tab.reset(extensions::TabHelper::CreateTabValue(source_contents));

    content::RenderFrameHost* rfh =
        content::RenderFrameHost::FromID(source_process_id, source_routing_id);
    if (rfh)
      source_frame_id = ExtensionApiFrameIdMap::GetFrameId(rfh);
  }

  std::unique_ptr<OpenChannelParams> params(new OpenChannelParams(
      source_process_id, source_routing_id, std::move(source_tab),
      source_frame_id, nullptr, receiver_port_id, source_extension_id,
      target_extension_id, source_url, channel_name, include_tls_channel_id,
      include_guest_process_info));

  pending_incognito_channels_[GET_CHANNEL_ID(params->receiver_port_id)] =
      PendingMessagesQueue();
  if (context->IsOffTheRecord() &&
      !AtomExtensionsBrowserClient::IsIncognitoEnabled(target_extension_id,
                                                                    context)) {
    // Give the user a chance to accept an incognito connection from the web if
    // they haven't already, with the conditions:
    // - Only for spanning-mode incognito. We don't want the complication of
    //   spinning up an additional process here which might need to do some
    //   setup that we're not expecting.
    // - Only for extensions that can't normally be enabled in incognito, since
    //   that surface (e.g. chrome://extensions) should be the only one for
    //   enabling in incognito. In practice this means platform apps only.
    if (!is_web_connection || IncognitoInfo::IsSplitMode(target_extension) ||
        util::CanBeIncognitoEnabled(target_extension)) {
      OnOpenChannelAllowed(std::move(params), false);
      return;
    }

    // If the target extension isn't even listening for connect/message events,
    // there is no need to go any further and the connection should be
    // rejected without showing a prompt. See http://crbug.com/442497
    EventRouter* event_router = EventRouter::Get(context);
    const char* const events[] = {"runtime.onConnectExternal",
                                  "runtime.onMessageExternal",
                                  "extension.onRequestExternal",
                                  nullptr};
    bool has_event_listener = false;
    for (const char* const* event = events; *event; event++) {
      has_event_listener |=
          event_router->ExtensionHasEventListener(target_extension_id, *event);
    }
    if (!has_event_listener) {
      OnOpenChannelAllowed(std::move(params), false);
      return;
    }

    return;
  }

  OnOpenChannelAllowed(std::move(params), true);
}

void MessageService::OpenChannelToNativeApp(
    int source_process_id,
    int source_routing_id,
    int receiver_port_id,
    const std::string& native_app_name) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  content::RenderFrameHost* source =
      content::RenderFrameHost::FromID(source_process_id, source_routing_id);
  if (!source)
    return;

  content::WebContents* web_contents =
      content::WebContents::FromRenderFrameHost(source);
  ExtensionWebContentsObserver* extension_web_contents_observer =
      web_contents ?
          ExtensionWebContentsObserver::GetForWebContents(web_contents) :
          nullptr;
  const Extension* extension =
      extension_web_contents_observer ?
          extension_web_contents_observer->GetExtensionFromFrame(source, true) :
          nullptr;

  bool has_permission = extension &&
      extension->permissions_data()->HasAPIPermission(
          APIPermission::kNativeMessaging);

  if (!has_permission) {
    DispatchOnDisconnect(source, receiver_port_id, kMissingPermissionError);
    return;
  }

  DispatchOnDisconnect(source, receiver_port_id, kProhibitedByPoliciesError);
}

void MessageService::OpenChannelToTab(int source_process_id,
                                      int source_routing_id,
                                      int receiver_port_id,
                                      int tab_id,
                                      int frame_id,
                                      const std::string& extension_id,
                                      const std::string& channel_name) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK_GE(frame_id, -1);

  content::RenderFrameHost* source =
      content::RenderFrameHost::FromID(source_process_id, source_routing_id);
  if (!source)
    return;

  BrowserContext* browser_context = source->GetProcess()->GetBrowserContext();

  WebContents* contents = TabHelper::GetTabById(tab_id, browser_context);
  std::unique_ptr<MessagePort> receiver;
  if (!contents || contents->GetController().NeedsReload()) {
    // The tab isn't loaded yet. Don't attempt to connect.
    DispatchOnDisconnect(
        source, receiver_port_id, kReceivingEndDoesntExistError);
    return;
  }

  // Frame ID -1 is every frame in the tab.
  bool include_child_frames = frame_id == -1;
  content::RenderFrameHost* receiver_rfh =
      include_child_frames
          ? contents->GetMainFrame()
          : ExtensionApiFrameIdMap::GetRenderFrameHostById(contents, frame_id);
  if (!receiver_rfh) {
    DispatchOnDisconnect(
        source, receiver_port_id, kReceivingEndDoesntExistError);
    return;
  }
  receiver.reset(
      new ExtensionMessagePort(weak_factory_.GetWeakPtr(),
                               receiver_port_id, extension_id, receiver_rfh,
                               include_child_frames));

  const Extension* extension = nullptr;
  if (!extension_id.empty()) {
    // Source extension == target extension so the extension must exist, or
    // where did the IPC come from?
    extension = ExtensionRegistry::Get(browser_context)->
        enabled_extensions().GetByID(extension_id);
    DCHECK(extension);
  }

  std::unique_ptr<OpenChannelParams> params(new OpenChannelParams(
      source_process_id,
      source_routing_id,
      // Source tab doesn't make sense
      std::unique_ptr<base::DictionaryValue>(),
                                            // for opening to tabs.
      -1,  // If there is no tab, then there is no frame either.
      receiver.release(), receiver_port_id, extension_id, extension_id,
      GURL(),  // Source URL doesn't make sense for opening to tabs.
      channel_name,
      false,    // Connections to tabs don't get TLS channel IDs.
      false));  // Connections to tabs aren't webview guests.
  OpenChannelImpl(contents->GetBrowserContext(), std::move(params), extension,
                  false /* did_enqueue */);
}

void MessageService::OpenChannelImpl(BrowserContext* browser_context,
                                     std::unique_ptr<OpenChannelParams> params,
                                     const Extension* target_extension,
                                     bool did_enqueue) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK_EQ(target_extension != nullptr, !params->target_extension_id.empty());

  content::RenderFrameHost* source =
      content::RenderFrameHost::FromID(params->source_process_id,
                                       params->source_routing_id);
  if (!source)
    return;  // Closed while in flight.

  if (!params->receiver || !params->receiver->IsValidPort()) {
    DispatchOnDisconnect(source, params->receiver_port_id,
                         kReceivingEndDoesntExistError);
    return;
  }

  std::unique_ptr<MessagePort> opener(
      new ExtensionMessagePort(weak_factory_.GetWeakPtr(),
                               GET_OPPOSITE_PORT_ID(params->receiver_port_id),
                               params->source_extension_id, source, false));
  if (!opener->IsValidPort())
    return;
  opener->OpenPort(params->source_process_id, params->source_routing_id);

  params->receiver->RemoveCommonFrames(*opener);
  if (!params->receiver->IsValidPort()) {
    opener->DispatchOnDisconnect(kReceivingEndDoesntExistError);
    return;
  }

  MessageChannel* channel(new MessageChannel());
  channel->opener.reset(opener.release());
  channel->receiver.reset(params->receiver.release());
  AddChannel(channel, params->receiver_port_id);

  int guest_process_id = content::ChildProcessHost::kInvalidUniqueID;
  int guest_render_frame_routing_id = MSG_ROUTING_NONE;

  // Send the connect event to the receiver.  Give it the opener's port ID (the
  // opener has the opposite port ID).
  channel->receiver->DispatchOnConnect(
      params->channel_name, std::move(params->source_tab),
      params->source_frame_id, guest_process_id, guest_render_frame_routing_id,
      params->source_extension_id, params->target_extension_id,
      params->source_url, params->tls_channel_id);

  // Report the event to the event router, if the target is an extension.
  //
  // First, determine what event this will be (runtime.onConnect vs
  // runtime.onMessage etc), and what the event target is (view vs background
  // page etc).
  //
  // Yes - even though this is opening a channel, they may actually be
  // runtime.onRequest/onMessage events because those single-use events are
  // built using the connect framework (see messaging.js).
  //
  // Likewise, if you're wondering about native messaging events, these are
  // only initiated *by* the extension, so aren't really events, just the
  // endpoint of a communication channel.
  if (target_extension) {
    events::HistogramValue histogram_value = events::UNKNOWN;
    bool is_external =
        params->source_extension_id != params->target_extension_id;
    if (params->channel_name == "chrome.runtime.onRequest") {
      histogram_value = is_external ? events::RUNTIME_ON_REQUEST_EXTERNAL
                                    : events::RUNTIME_ON_REQUEST;
    } else if (params->channel_name == "chrome.runtime.onMessage") {
      histogram_value = is_external ? events::RUNTIME_ON_MESSAGE_EXTERNAL
                                    : events::RUNTIME_ON_MESSAGE;
    } else {
      histogram_value = is_external ? events::RUNTIME_ON_CONNECT_EXTERNAL
                                    : events::RUNTIME_ON_CONNECT;
    }
    EventRouter::Get(browser_context)
        ->ReportEvent(histogram_value, target_extension, did_enqueue);
  }

  // Keep both ends of the channel alive until the channel is closed.
  channel->opener->IncrementLazyKeepaliveCount();
  channel->receiver->IncrementLazyKeepaliveCount();
}

void MessageService::AddChannel(MessageChannel* channel, int receiver_port_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  int channel_id = GET_CHANNEL_ID(receiver_port_id);
  CHECK(channels_.find(channel_id) == channels_.end());
  channels_[channel_id] = channel;
  pending_lazy_background_page_channels_.erase(channel_id);
}

void MessageService::OpenPort(int port_id, int process_id, int routing_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(!IS_OPENER_PORT_ID(port_id));

  int channel_id = GET_CHANNEL_ID(port_id);
  MessageChannelMap::iterator it = channels_.find(channel_id);
  if (it == channels_.end())
    return;

  it->second->receiver->OpenPort(process_id, routing_id);
}

void MessageService::ClosePort(
    int port_id, int process_id, int routing_id, bool force_close) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  ClosePortImpl(port_id, process_id, routing_id, force_close, std::string());
}

void MessageService::CloseChannel(int port_id,
                                  const std::string& error_message) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  ClosePortImpl(port_id, content::ChildProcessHost::kInvalidUniqueID,
                MSG_ROUTING_NONE, true, error_message);
}

void MessageService::ClosePortImpl(int port_id,
                                   int process_id,
                                   int routing_id,
                                   bool force_close,
                                   const std::string& error_message) {
  // Note: The channel might be gone already, if the other side closed first.
  int channel_id = GET_CHANNEL_ID(port_id);
  MessageChannelMap::iterator it = channels_.find(channel_id);
  if (it == channels_.end()) {
    PendingLazyBackgroundPageChannelMap::iterator pending =
        pending_lazy_background_page_channels_.find(channel_id);
    if (pending != pending_lazy_background_page_channels_.end()) {
      lazy_background_task_queue_->AddPendingTask(
          pending->second.first, pending->second.second,
          base::Bind(&MessageService::PendingLazyBackgroundPageClosePort,
                     weak_factory_.GetWeakPtr(), port_id, process_id,
                     routing_id, force_close, error_message));
    }
    return;
  }

  // The difference between closing a channel and port is that closing a port
  // does not necessarily have to destroy the channel if there are multiple
  // receivers, whereas closing a channel always forces all ports to be closed.
  if (force_close) {
    CloseChannelImpl(it, port_id, error_message, true);
  } else if (IS_OPENER_PORT_ID(port_id)) {
    it->second->opener->ClosePort(process_id, routing_id);
  } else {
    it->second->receiver->ClosePort(process_id, routing_id);
  }
}

void MessageService::CloseChannelImpl(
    MessageChannelMap::iterator channel_iter,
    int closing_port_id,
    const std::string& error_message,
    bool notify_other_port) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  MessageChannel* channel = channel_iter->second;
  // Remove from map to make sure that it is impossible for CloseChannelImpl to
  // run twice for the same channel.
  channels_.erase(channel_iter);

  // Notify the other side.
  if (notify_other_port) {
    MessagePort* port = IS_OPENER_PORT_ID(closing_port_id) ?
        channel->receiver.get() : channel->opener.get();
    port->DispatchOnDisconnect(error_message);
  }

  // Balance the IncrementLazyKeepaliveCount() in OpenChannelImpl.
  channel->opener->DecrementLazyKeepaliveCount();
  channel->receiver->DecrementLazyKeepaliveCount();

  delete channel;
}

void MessageService::PostMessage(int source_port_id, const Message& message) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  int channel_id = GET_CHANNEL_ID(source_port_id);
  MessageChannelMap::iterator iter = channels_.find(channel_id);
  if (iter == channels_.end()) {
    // If this channel is pending, queue up the PostMessage to run once
    // the channel opens.
    EnqueuePendingMessage(source_port_id, channel_id, message);
    return;
  }

  DispatchMessage(source_port_id, iter->second, message);
}

void MessageService::EnqueuePendingMessage(int source_port_id,
                                           int channel_id,
                                           const Message& message) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  PendingChannelMap::iterator pending_for_incognito =
      pending_incognito_channels_.find(channel_id);
  if (pending_for_incognito != pending_incognito_channels_.end()) {
    pending_for_incognito->second.push_back(
        PendingMessage(source_port_id, message));
    // A channel should only be holding pending messages because it is in one
    // of these states.
    DCHECK(!ContainsKey(pending_tls_channel_id_channels_, channel_id));
    DCHECK(!ContainsKey(pending_lazy_background_page_channels_, channel_id));
    return;
  }
  PendingChannelMap::iterator pending_for_tls_channel_id =
      pending_tls_channel_id_channels_.find(channel_id);
  if (pending_for_tls_channel_id != pending_tls_channel_id_channels_.end()) {
    pending_for_tls_channel_id->second.push_back(
        PendingMessage(source_port_id, message));
    // A channel should only be holding pending messages because it is in one
    // of these states.
    DCHECK(!ContainsKey(pending_lazy_background_page_channels_, channel_id));
    return;
  }
  EnqueuePendingMessageForLazyBackgroundLoad(source_port_id,
                                             channel_id,
                                             message);
}

void MessageService::EnqueuePendingMessageForLazyBackgroundLoad(
    int source_port_id,
    int channel_id,
    const Message& message) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  PendingLazyBackgroundPageChannelMap::iterator pending =
      pending_lazy_background_page_channels_.find(channel_id);
  if (pending != pending_lazy_background_page_channels_.end()) {
    lazy_background_task_queue_->AddPendingTask(
        pending->second.first, pending->second.second,
        base::Bind(&MessageService::PendingLazyBackgroundPagePostMessage,
                   weak_factory_.GetWeakPtr(), source_port_id, message));
  }
}

void MessageService::DispatchMessage(int source_port_id,
                                     MessageChannel* channel,
                                     const Message& message) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  // Figure out which port the ID corresponds to.
  int dest_port_id = GET_OPPOSITE_PORT_ID(source_port_id);
  MessagePort* port = IS_OPENER_PORT_ID(dest_port_id) ?
      channel->opener.get() : channel->receiver.get();

  port->DispatchOnMessage(message);
}

bool MessageService::MaybeAddPendingLazyBackgroundPageOpenChannelTask(
    BrowserContext* context,
    const Extension* extension,
    std::unique_ptr<OpenChannelParams>* params,
    const PendingMessagesQueue& pending_messages) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  if (!BackgroundInfo::HasLazyBackgroundPage(extension))
    return false;

  // If the extension uses spanning incognito mode, make sure we're always
  // using the original browser_context since that is what the extension process
  // will use.
  if (!IncognitoInfo::IsSplitMode(extension))
    context = ExtensionsBrowserClient::Get()->GetOriginalContext(context);

  if (!lazy_background_task_queue_->ShouldEnqueueTask(context, extension))
    return false;

  int channel_id = GET_CHANNEL_ID((*params)->receiver_port_id);
  pending_lazy_background_page_channels_[channel_id] =
      PendingLazyBackgroundPageChannel(context, extension->id());
  int source_id = (*params)->source_process_id;
  lazy_background_task_queue_->AddPendingTask(
      context, extension->id(),
      base::Bind(&MessageService::PendingLazyBackgroundPageOpenChannel,
                 weak_factory_.GetWeakPtr(), base::Passed(params), source_id));

  for (const PendingMessage& message : pending_messages) {
    EnqueuePendingMessageForLazyBackgroundLoad(message.first, channel_id,
                                               message.second);
  }
  return true;
}

void
MessageService::OnOpenChannelAllowed(std::unique_ptr<OpenChannelParams>params,
                                     bool allowed) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  int channel_id = GET_CHANNEL_ID(params->receiver_port_id);

  PendingChannelMap::iterator pending_for_incognito =
      pending_incognito_channels_.find(channel_id);
  if (pending_for_incognito == pending_incognito_channels_.end()) {
    NOTREACHED();
    return;
  }
  PendingMessagesQueue pending_messages;
  pending_messages.swap(pending_for_incognito->second);
  pending_incognito_channels_.erase(pending_for_incognito);

  // Re-lookup the source process since it may no longer be valid.
  content::RenderFrameHost* source =
      content::RenderFrameHost::FromID(params->source_process_id,
                                       params->source_routing_id);
  if (!source) {
    return;
  }

  if (!allowed) {
    DispatchOnDisconnect(source, params->receiver_port_id,
                         kReceivingEndDoesntExistError);
    return;
  }

  BrowserContext* context = source->GetProcess()->GetBrowserContext();

  if (content::RenderProcessHost* extension_process =
      GetExtensionProcess(context, params->target_extension_id)) {
    params->receiver.reset(
        new ExtensionMessagePort(
            weak_factory_.GetWeakPtr(), params->receiver_port_id,
            params->target_extension_id, extension_process));
  } else {
    params->receiver.reset();
  }

  // If the target requests the TLS channel id, begin the lookup for it.
  // The target might also be a lazy background page, checked next, but the
  // loading of lazy background pages continues asynchronously, so enqueue
  // messages awaiting TLS channel ID first.
  if (params->include_tls_channel_id) {
    // Transfer pending messages to the next pending channel list.
    pending_tls_channel_id_channels_[channel_id].swap(pending_messages);
    // Capture this reference before params is invalidated by base::Passed().
    const GURL& source_url = params->source_url;
    property_provider_.GetChannelID(context, source_url,
        base::Bind(&MessageService::GotChannelID, weak_factory_.GetWeakPtr(),
                   base::Passed(&params)));
    return;
  }

  ExtensionRegistry* registry = ExtensionRegistry::Get(context);
  const Extension* target_extension =
      registry->enabled_extensions().GetByID(params->target_extension_id);
  if (!target_extension) {
    DispatchOnDisconnect(source, params->receiver_port_id,
                         kReceivingEndDoesntExistError);
    return;
  }

  // The target might be a lazy background page. In that case, we have to check
  // if it is loaded and ready, and if not, queue up the task and load the
  // page.
  if (!MaybeAddPendingLazyBackgroundPageOpenChannelTask(
          context, target_extension, &params, pending_messages)) {
    OpenChannelImpl(context, std::move(params), target_extension,
                    false /* did_enqueue */);
    DispatchPendingMessages(pending_messages, channel_id);
  }
}

void MessageService::GotChannelID(std::unique_ptr<OpenChannelParams> params,
                                  const std::string& tls_channel_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  params->tls_channel_id.assign(tls_channel_id);
  int channel_id = GET_CHANNEL_ID(params->receiver_port_id);

  PendingChannelMap::iterator pending_for_tls_channel_id =
      pending_tls_channel_id_channels_.find(channel_id);
  if (pending_for_tls_channel_id == pending_tls_channel_id_channels_.end()) {
    NOTREACHED();
    return;
  }
  PendingMessagesQueue pending_messages;
  pending_messages.swap(pending_for_tls_channel_id->second);
  pending_tls_channel_id_channels_.erase(pending_for_tls_channel_id);

  // Re-lookup the source process since it may no longer be valid.
  content::RenderFrameHost* source =
      content::RenderFrameHost::FromID(params->source_process_id,
                                       params->source_routing_id);
  if (!source) {
    return;
  }

  BrowserContext* context = source->GetProcess()->GetBrowserContext();
  ExtensionRegistry* registry = ExtensionRegistry::Get(context);
  const Extension* target_extension =
      registry->enabled_extensions().GetByID(params->target_extension_id);
  if (!target_extension) {
    DispatchOnDisconnect(source, params->receiver_port_id,
                         kReceivingEndDoesntExistError);
    return;
  }

  if (!MaybeAddPendingLazyBackgroundPageOpenChannelTask(
          context, target_extension, &params, pending_messages)) {
    OpenChannelImpl(context, std::move(params), target_extension,
                    false /* did_enqueue */);
    DispatchPendingMessages(pending_messages, channel_id);
  }
}

void MessageService::PendingLazyBackgroundPageOpenChannel(
    std::unique_ptr<OpenChannelParams> params,
    int source_process_id,
    ExtensionHost* host) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  if (!host)
    return;  // TODO(mpcomplete): notify source of disconnect?

  params->receiver.reset(
      new ExtensionMessagePort(
          weak_factory_.GetWeakPtr(), params->receiver_port_id,
          params->target_extension_id, host->render_process_host()));
  OpenChannelImpl(host->browser_context(), std::move(params), host->extension(),
                  true /* did_enqueue */);
}

void MessageService::DispatchOnDisconnect(content::RenderFrameHost* source,
                                          int port_id,
                                          const std::string& error_message) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  ExtensionMessagePort port(weak_factory_.GetWeakPtr(),
                            GET_OPPOSITE_PORT_ID(port_id), "", source, false);
  if (!port.IsValidPort())
    return;
  port.DispatchOnDisconnect(error_message);
}

void MessageService::DispatchPendingMessages(const PendingMessagesQueue& queue,
                                             int channel_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  MessageChannelMap::iterator channel_iter = channels_.find(channel_id);
  if (channel_iter != channels_.end()) {
    for (const PendingMessage& message : queue) {
      DispatchMessage(message.first, channel_iter->second, message.second);
    }
  }
}

}  // namespace extensions
