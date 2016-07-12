// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_EXTENSIONS_API_MESSAGING_MESSAGE_SERVICE_H_
#define ATOM_BROWSER_EXTENSIONS_API_MESSAGING_MESSAGE_SERVICE_H_

#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include "atom/browser/extensions/api/messaging/message_property_provider.h"
#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/values.h"
#include "extensions/browser/api/messaging/native_message_host.h"
#include "extensions/browser/browser_context_keyed_api_factory.h"
#include "extensions/common/api/messaging/message.h"

class GURL;
class Profile;

namespace content {
class BrowserContext;
}

namespace extensions {
class Extension;
class ExtensionHost;
class LazyBackgroundTaskQueue;

// This class manages message and event passing between renderer processes.
// It maintains a list of processes that are listening to events and a set of
// open channels.
//
// Messaging works this way:
// - An extension-owned script context (like a background page or a content
//   script) adds an event listener to the "onConnect" event.
// - Another context calls "runtime.connect()" to open a channel to the
// extension process, or an extension context calls "tabs.connect(tabId)" to
// open a channel to the content scripts for the given tab.  The EMS notifies
// the target process/tab, which then calls the onConnect event in every
// context owned by the connecting extension in that process/tab.
// - Once the channel is established, either side can call postMessage to send
// a message to the opposite side of the channel, which may have multiple
// listeners.
//
// Terminology:
// channel: connection between two ports
// port: One sender or receiver tied to one or more RenderFrameHost instances.
class MessageService : public BrowserContextKeyedAPI {
 public:
  // A messaging channel. Note that the opening port can be the same as the
  // receiver, if an extension background page wants to talk to its tab (for
  // example).
  struct MessageChannel;

  // One side of the communication handled by extensions::MessageService.
  class MessagePort {
   public:
    virtual ~MessagePort() {}

    // Called right before a channel is created for this MessagePort and |port|.
    // This allows us to ensure that the ports have no RenderFrameHost instances
    // in common.
    virtual void RemoveCommonFrames(const MessagePort& port);

    // Check whether the given RenderFrameHost is associated with this port.
    virtual bool HasFrame(content::RenderFrameHost* rfh) const;

    // Called right before a port is connected to a channel. If false, the port
    // is not used and the channel is closed.
    virtual bool IsValidPort() = 0;

    // Notify the port that the channel has been opened.
    virtual void DispatchOnConnect(const std::string& channel_name,
                                   std::unique_ptr<base::DictionaryValue>
                                       source_tab,
                                   int source_frame_id,
                                   int guest_process_id,
                                   int guest_render_frame_routing_id,
                                   const std::string& source_extension_id,
                                   const std::string& target_extension_id,
                                   const GURL& source_url,
                                   const std::string& tls_channel_id) {}

    // Notify the port that the channel has been closed. If |error_message| is
    // non-empty, it indicates an error occurred while opening the connection.
    virtual void DispatchOnDisconnect(const std::string& error_message) {}

    // Dispatch a message to this end of the communication.
    virtual void DispatchOnMessage(const Message& message) = 0;

    // Mark the port as opened by the specific frame.
    virtual void OpenPort(int process_id, int routing_id) {}

    // Close the port for the given frame.
    virtual void ClosePort(int process_id, int routing_id) {}

    // MessagePorts that target extensions will need to adjust their keepalive
    // counts for their lazy background page.
    virtual void IncrementLazyKeepaliveCount() {}
    virtual void DecrementLazyKeepaliveCount() {}

   protected:
    MessagePort() {}

   private:
    DISALLOW_COPY_AND_ASSIGN(MessagePort);
  };

  enum PolicyPermission {
    DISALLOW,           // The host is not allowed.
    ALLOW_SYSTEM_ONLY,  // Allowed only when installed on system level.
    ALLOW_ALL,          // Allowed when installed on system or user level.
  };

  static PolicyPermission IsNativeMessagingHostAllowed(
      const PrefService* pref_service,
      const std::string& native_host_name);

  // Allocates a pair of port ids.
  // NOTE: this can be called from any thread.
  static void AllocatePortIdPair(int* port1, int* port2);

  explicit MessageService(content::BrowserContext* context);
  ~MessageService() override;

  // BrowserContextKeyedAPI implementation.
  static BrowserContextKeyedAPIFactory<MessageService>* GetFactoryInstance();

  // Convenience method to get the MessageService for a browser context.
  static MessageService* Get(content::BrowserContext* context);

  // Given an extension's ID, opens a channel between the given renderer "port"
  // and every listening context owned by that extension. |channel_name| is
  // an optional identifier for use by extension developers.
  void OpenChannelToExtension(
      int source_process_id, int source_routing_id, int receiver_port_id,
      const std::string& source_extension_id,
      const std::string& target_extension_id,
      const GURL& source_url,
      const std::string& channel_name,
      bool include_tls_channel_id);

  // Same as above, but opens a channel to the tab with the given ID.  Messages
  // are restricted to that tab, so if there are multiple tabs in that process,
  // only the targeted tab will receive messages.
  void OpenChannelToTab(int source_process_id,
                        int source_routing_id,
                        int receiver_port_id,
                        int tab_id,
                        int frame_id,
                        const std::string& extension_id,
                        const std::string& channel_name);

  void OpenChannelToNativeApp(
      int source_process_id,
      int source_routing_id,
      int receiver_port_id,
      const std::string& source_extension_id,
      const std::string& native_app_name);

  // Mark the given port as opened by the frame identified by
  // (process_id, routing_id).
  void OpenPort(int port_id, int process_id, int routing_id);

  // Closes the given port in the given frame. If this was the last frame or if
  // |force_close| is true, then the other side is closed as well.
  void ClosePort(int port_id, int process_id, int routing_id, bool force_close);

  // Closes the message channel associated with the given port, and notifies
  // the other side.
  void CloseChannel(int port_id, const std::string& error_message);

  // Enqueues a message on a pending channel, or sends a message to the given
  // port if the channel isn't pending.
  void PostMessage(int port_id, const Message& message);

 private:
  friend class MockMessageService;
  friend class BrowserContextKeyedAPIFactory<MessageService>;
  struct OpenChannelParams;

  // A map of channel ID to its channel object.
  typedef std::map<int, MessageChannel*> MessageChannelMap;

  typedef std::pair<int, Message> PendingMessage;
  typedef std::vector<PendingMessage> PendingMessagesQueue;
  // A set of channel IDs waiting to complete opening, and any pending messages
  // queued to be sent on those channels.
  typedef std::map<int, PendingMessagesQueue> PendingChannelMap;

  // A map of channel ID to information about the extension that is waiting
  // for that channel to open. Used for lazy background pages.
  typedef std::string ExtensionID;
  typedef std::pair<content::BrowserContext*, ExtensionID>
      PendingLazyBackgroundPageChannel;
  typedef std::map<int, PendingLazyBackgroundPageChannel>
      PendingLazyBackgroundPageChannelMap;

  // Common implementation for opening a channel configured by |params|.
  //
  // |target_extension| will be non-null if |params->target_extension_id| is
  // non-empty, that is, if the target is an extension, it must exist.
  //
  // |did_enqueue| will be true if the channel opening was delayed while
  // waiting for an event page to start, false otherwise.
  void OpenChannelImpl(content::BrowserContext* browser_context,
                       std::unique_ptr<OpenChannelParams> params,
                       const Extension* target_extension,
                       bool did_enqueue);

  void ClosePortImpl(int port_id,
                     int process_id,
                     int routing_id,
                     bool force_close,
                     const std::string& error_message);

  void CloseChannelImpl(MessageChannelMap::iterator channel_iter,
                        int port_id,
                        const std::string& error_message,
                        bool notify_other_port);

  // Have MessageService take ownership of |channel|, and remove any pending
  // channels with the same id.
  void AddChannel(MessageChannel* channel, int receiver_port_id);

  // If the channel is being opened from an incognito tab the user must allow
  // the connection.
  void OnOpenChannelAllowed(std::unique_ptr<OpenChannelParams> params,
      bool allowed);
  void GotChannelID(std::unique_ptr<OpenChannelParams> params,
                    const std::string& tls_channel_id);

  // Enqueues a message on a pending channel.
  void EnqueuePendingMessage(int port_id, int channel_id,
                             const Message& message);

  // Enqueues a message on a channel pending on a lazy background page load.
  void EnqueuePendingMessageForLazyBackgroundLoad(int port_id,
                                                  int channel_id,
                                                  const Message& message);

  // Immediately sends a message to the given port.
  void DispatchMessage(int port_id, MessageChannel* channel,
                       const Message& message);

  // Potentially registers a pending task with the LazyBackgroundTaskQueue
  // to open a channel. Returns true if a task was queued.
  // Takes ownership of |params| if true is returned.
  bool MaybeAddPendingLazyBackgroundPageOpenChannelTask(
      content::BrowserContext* context,
      const Extension* extension,
      std::unique_ptr<OpenChannelParams>* params,
      const PendingMessagesQueue& pending_messages);

  // Callbacks for LazyBackgroundTaskQueue tasks. The queue passes in an
  // ExtensionHost to its task callbacks, though some of our callbacks don't
  // use that argument.
  void PendingLazyBackgroundPageOpenChannel(
      std::unique_ptr<OpenChannelParams> params,
      int source_process_id,
      extensions::ExtensionHost* host);
  void PendingLazyBackgroundPageClosePort(int port_id,
                                          int process_id,
                                          int routing_id,
                                          bool force_close,
                                          const std::string& error_message,
                                          extensions::ExtensionHost* host) {
    if (host)
      ClosePortImpl(port_id, process_id, routing_id, force_close,
                    error_message);
  }
  void PendingLazyBackgroundPagePostMessage(int port_id,
                                            const Message& message,
                                            extensions::ExtensionHost* host) {
    if (host)
      PostMessage(port_id, message);
  }

  // Immediate dispatches a disconnect to |source| for |port_id|. Sets source's
  // runtime.lastMessage to |error_message|, if any.
  void DispatchOnDisconnect(content::RenderFrameHost* source,
                            int port_id,
                            const std::string& error_message);

  void DispatchPendingMessages(const PendingMessagesQueue& queue,
                               int channel_id);

  // BrowserContextKeyedAPI implementation.
  static const char* service_name() {
    return "MessageService";
  }
  static const bool kServiceRedirectedInIncognito = true;
  static const bool kServiceIsCreatedWithBrowserContext = false;
  static const bool kServiceIsNULLWhileTesting = true;

  MessageChannelMap channels_;
  // A set of channel IDs waiting for TLS channel IDs to complete opening, and
  // any pending messages queued to be sent on those channels. This and the
  // following two maps form a pipeline where messages are queued before the
  // channel they are addressed to is ready.
  PendingChannelMap pending_tls_channel_id_channels_;
  // A set of channel IDs waiting for user permission to cross the border
  // between an incognito page and an app or extension, and any pending messages
  // queued to be sent on those channels.
  PendingChannelMap pending_incognito_channels_;
  PendingLazyBackgroundPageChannelMap pending_lazy_background_page_channels_;
  MessagePropertyProvider property_provider_;

  // Weak pointer. Guaranteed to outlive this class.
  LazyBackgroundTaskQueue* lazy_background_task_queue_;

  base::WeakPtrFactory<MessageService> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(MessageService);
};

}  // namespace extensions

#endif  // ATOM_BROWSER_EXTENSIONS_API_MESSAGING_MESSAGE_SERVICE_H_
