// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "atom/browser/extensions/api/messaging/extension_message_port.h"

#include "base/scoped_observer.h"
#include "content/public/browser/interstitial_page.h"
#include "content/public/browser/navigation_details.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/web_contents.h"
#include "extensions/browser/extension_host.h"
#include "extensions/browser/process_manager.h"
#include "extensions/browser/process_manager_observer.h"
#include "extensions/common/extension_messages.h"
#include "extensions/common/manifest_handlers/background_info.h"

namespace extensions {

const char kReceivingEndDoesntExistError[] =
    "Could not establish connection. Receiving end does not exist.";

// Helper class to detect when frames are destroyed.
class ExtensionMessagePort::FrameTracker : public content::WebContentsObserver,
                                           public ProcessManagerObserver {
 public:
  explicit FrameTracker(ExtensionMessagePort* port)
      : pm_observer_(this), port_(port), interstitial_frame_(nullptr) {}
  ~FrameTracker() override {}

  void TrackExtensionProcessFrames() {
    pm_observer_.Add(ProcessManager::Get(port_->browser_context_));
  }

  void TrackTabFrames(content::WebContents* tab) {
    Observe(tab);
  }

  void TrackInterstitialFrame(content::WebContents* tab,
                              content::RenderFrameHost* interstitial_frame) {
    // |tab| should never be nullptr, because an interstitial's lifetime is
    // tied to a tab. This is a CHECK, not a DCHECK because we really need an
    // observer subject to detect frame removal (via DidDetachInterstitialPage).
    CHECK(tab);
    DCHECK(interstitial_frame);
    interstitial_frame_ = interstitial_frame;
    Observe(tab);
  }

 private:
  // content::WebContentsObserver overrides:
  void RenderFrameDeleted(content::RenderFrameHost* render_frame_host)
      override {
    port_->UnregisterFrame(render_frame_host);
  }

  void DidNavigateAnyFrame(content::RenderFrameHost* render_frame_host,
                           const content::LoadCommittedDetails& details,
                           const content::FrameNavigateParams&) override {
    if (!details.is_in_page)
      port_->UnregisterFrame(render_frame_host);
  }

  void DidDetachInterstitialPage() override {
    if (interstitial_frame_)
      port_->UnregisterFrame(interstitial_frame_);
  }

  // extensions::ProcessManagerObserver overrides:
  void OnExtensionFrameUnregistered(
      const std::string& extension_id,
      content::RenderFrameHost* render_frame_host) override {
    if (extension_id == port_->extension_id_)
      port_->UnregisterFrame(render_frame_host);
  }

  ScopedObserver<ProcessManager, ProcessManagerObserver> pm_observer_;
  ExtensionMessagePort* port_;  // Owns this FrameTracker.

  // Set to the main frame of an interstitial if we are tracking an interstitial
  // page, because RenderFrameDeleted is never triggered for frames in an
  // interstitial (and we only support tracking the interstitial's main frame).
  content::RenderFrameHost* interstitial_frame_;

  DISALLOW_COPY_AND_ASSIGN(FrameTracker);
};

ExtensionMessagePort::ExtensionMessagePort(
    base::WeakPtr<MessageService> message_service,
    int port_id,
    const std::string& extension_id,
    content::RenderProcessHost* extension_process)
    : weak_message_service_(message_service),
      port_id_(port_id),
      extension_id_(extension_id),
      browser_context_(extension_process->GetBrowserContext()),
      extension_process_(extension_process),
      did_create_port_(false),
      background_host_ptr_(nullptr),
      frame_tracker_(new FrameTracker(this)) {
  auto all_hosts = ProcessManager::Get(browser_context_)
                       ->GetRenderFrameHostsForExtension(extension_id);
  for (content::RenderFrameHost* rfh : all_hosts)
    RegisterFrame(rfh);

  frame_tracker_->TrackExtensionProcessFrames();
}

ExtensionMessagePort::ExtensionMessagePort(
    base::WeakPtr<MessageService> message_service,
    int port_id,
    const std::string& extension_id,
    content::RenderFrameHost* rfh,
    bool include_child_frames)
    : weak_message_service_(message_service),
      port_id_(port_id),
      extension_id_(extension_id),
      browser_context_(rfh->GetProcess()->GetBrowserContext()),
      extension_process_(nullptr),
      did_create_port_(false),
      background_host_ptr_(nullptr),
      frame_tracker_(new FrameTracker(this)) {
  content::WebContents* tab = content::WebContents::FromRenderFrameHost(rfh);
  if (!tab) {
    content::InterstitialPage* interstitial =
        content::InterstitialPage::FromRenderFrameHost(rfh);
    // A RenderFrameHost must be hosted in a WebContents or InterstitialPage.
    CHECK(interstitial);

    // Only the main frame of an interstitial is supported, because frames in
    // the interstitial do not trigger RenderFrameCreated / RenderFrameDeleted
    // on WebContentObservers. Consequently, (1) we cannot detect removal of
    // RenderFrameHosts, and (2) even if the RenderFrameDeleted is propagated,
    // then WebContentsObserverSanityChecker triggers a CHECK when it detects
    // frame notifications without a corresponding RenderFrameCreated.
    if (!rfh->GetParent()) {
      // It is safe to pass the interstitial's WebContents here because we only
      // use it to observe DidDetachInterstitialPage.
      frame_tracker_->TrackInterstitialFrame(interstitial->GetWebContents(),
                                             rfh);
      RegisterFrame(rfh);
    }
    return;
  }

  frame_tracker_->TrackTabFrames(tab);
  if (include_child_frames) {
    tab->ForEachFrame(base::Bind(&ExtensionMessagePort::RegisterFrame,
                                 base::Unretained(this)));
  } else {
    RegisterFrame(rfh);
  }
}

ExtensionMessagePort::~ExtensionMessagePort() {}

void ExtensionMessagePort::RemoveCommonFrames(const MessagePort& port) {
  // Avoid overlap in the set of frames to make sure that it does not matter
  // when UnregisterFrame is called.
  for (std::set<content::RenderFrameHost*>::iterator it = frames_.begin();
       it != frames_.end(); ) {
    if (port.HasFrame(*it)) {
      frames_.erase(it++);
    } else {
      ++it;
    }
  }
}

bool ExtensionMessagePort::HasFrame(content::RenderFrameHost* rfh) const {
  return frames_.find(rfh) != frames_.end();
}

bool ExtensionMessagePort::IsValidPort() {
  return !frames_.empty();
}

void ExtensionMessagePort::DispatchOnConnect(
    const std::string& channel_name,
    std::unique_ptr<base::DictionaryValue> source_tab,
    int source_frame_id,
    int guest_process_id,
    int guest_render_frame_routing_id,
    const std::string& source_extension_id,
    const std::string& target_extension_id,
    const GURL& source_url,
    const std::string& tls_channel_id) {
  ExtensionMsg_TabConnectionInfo source;
  if (source_tab)
    source.tab.Swap(source_tab.get());
  source.frame_id = source_frame_id;

  ExtensionMsg_ExternalConnectionInfo info;
  info.target_id = target_extension_id;
  info.source_id = source_extension_id;
  info.source_url = source_url;
  info.guest_process_id = guest_process_id;
  info.guest_render_frame_routing_id = guest_render_frame_routing_id;

  SendToPort(base::WrapUnique(new ExtensionMsg_DispatchOnConnect(
      MSG_ROUTING_NONE, port_id_, channel_name, source, info, tls_channel_id)));
}

void ExtensionMessagePort::DispatchOnDisconnect(
    const std::string& error_message) {
  SendToPort(base::WrapUnique(new ExtensionMsg_DispatchOnDisconnect(
      MSG_ROUTING_NONE, port_id_, error_message)));
}

void ExtensionMessagePort::DispatchOnMessage(const Message& message) {
  SendToPort(base::WrapUnique(new ExtensionMsg_DeliverMessage(
      MSG_ROUTING_NONE, port_id_, message)));
}

void ExtensionMessagePort::IncrementLazyKeepaliveCount() {
  ProcessManager* pm = ProcessManager::Get(browser_context_);
  ExtensionHost* host = pm->GetBackgroundHostForExtension(extension_id_);
  if (host && BackgroundInfo::HasLazyBackgroundPage(host->extension()))
    pm->IncrementLazyKeepaliveCount(host->extension());

  // Keep track of the background host, so when we decrement, we only do so if
  // the host hasn't reloaded.
  background_host_ptr_ = host;
}

void ExtensionMessagePort::DecrementLazyKeepaliveCount() {
  ProcessManager* pm = ProcessManager::Get(browser_context_);
  ExtensionHost* host = pm->GetBackgroundHostForExtension(extension_id_);
  if (host && host == background_host_ptr_)
    pm->DecrementLazyKeepaliveCount(host->extension());
}

void ExtensionMessagePort::OpenPort(int process_id, int routing_id) {
  DCHECK(routing_id != MSG_ROUTING_NONE || extension_process_);

  did_create_port_ = true;
}

void ExtensionMessagePort::ClosePort(int process_id, int routing_id) {
  if (routing_id == MSG_ROUTING_NONE) {
    // The only non-frame-specific message is the response to an unhandled
    // onConnect event in the extension process.
    DCHECK(extension_process_);
    frames_.clear();
    CloseChannel();
    return;
  }

  content::RenderFrameHost* rfh =
      content::RenderFrameHost::FromID(process_id, routing_id);
  if (rfh)
    UnregisterFrame(rfh);
}

void ExtensionMessagePort::CloseChannel() {
  std::string error_message = did_create_port_ ? std::string() :
      kReceivingEndDoesntExistError;
  if (weak_message_service_)
    weak_message_service_->CloseChannel(port_id_, error_message);
}

void ExtensionMessagePort::RegisterFrame(content::RenderFrameHost* rfh) {
  // Only register a RenderFrameHost whose RenderFrame has been created, to
  // ensure that we are notified of frame destruction. Without this check,
  // |frames_| can eventually contain a stale pointer because RenderFrameDeleted
  // is not triggered for |rfh|.
  if (rfh->IsRenderFrameLive())
    frames_.insert(rfh);
}

void ExtensionMessagePort::UnregisterFrame(content::RenderFrameHost* rfh) {
  if (frames_.erase(rfh) != 0 && frames_.empty())
    CloseChannel();
}

void ExtensionMessagePort::SendToPort(std::unique_ptr<IPC::Message> msg) {
  DCHECK_GT(frames_.size(), 0UL);
  if (extension_process_) {
    // All extension frames reside in the same process, so we can just send a
    // single IPC message to the extension process as an optimization.
    // The frame tracking is then only used to make sure that the port gets
    // closed when all frames have closed / reloaded.
    msg->set_routing_id(MSG_ROUTING_CONTROL);
    extension_process_->Send(msg.release());
    return;
  }
  for (content::RenderFrameHost* rfh : frames_) {
    IPC::Message* msg_copy = new IPC::Message(*msg.get());
    msg_copy->set_routing_id(rfh->GetRoutingID());
    rfh->Send(msg_copy);
  }
}

}  // namespace extensions
