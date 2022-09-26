// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/browser/extensions/electron_messaging_delegate.h"

#include <memory>
#include <utility>

#include "base/callback.h"
#include "base/logging.h"
#include "build/build_config.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "electron/shell/common/extensions/api/tabs.h"
#include "extensions/browser/api/messaging/extension_message_port.h"
#include "extensions/browser/api/messaging/native_message_host.h"
#include "extensions/browser/extension_api_frame_id_map.h"
#include "extensions/browser/pref_names.h"
#include "extensions/common/api/messaging/port_id.h"
#include "extensions/common/extension.h"
#include "shell/browser/api/electron_api_web_contents.h"
#include "ui/gfx/native_widget_types.h"
#include "url/gurl.h"

namespace extensions {

ElectronMessagingDelegate::ElectronMessagingDelegate() = default;
ElectronMessagingDelegate::~ElectronMessagingDelegate() = default;

MessagingDelegate::PolicyPermission
ElectronMessagingDelegate::IsNativeMessagingHostAllowed(
    content::BrowserContext* browser_context,
    const std::string& native_host_name) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  return PolicyPermission::DISALLOW;
}

absl::optional<base::Value::Dict> ElectronMessagingDelegate::MaybeGetTabInfo(
    content::WebContents* web_contents) {
  if (web_contents) {
    auto* api_contents = electron::api::WebContents::From(web_contents);
    if (api_contents) {
      api::tabs::Tab tab;
      tab.id = api_contents->ID();
      tab.url = api_contents->GetURL().spec();
      tab.title = base::UTF16ToUTF8(api_contents->GetTitle());
      tab.audible = api_contents->IsCurrentlyAudible();
      return std::move(tab.ToValue()->GetDict());
    }
  }
  return absl::nullopt;
}

content::WebContents* ElectronMessagingDelegate::GetWebContentsByTabId(
    content::BrowserContext* browser_context,
    int tab_id) {
  auto* contents = electron::api::WebContents::FromID(tab_id);
  if (!contents) {
    return nullptr;
  }
  return contents->web_contents();
}

std::unique_ptr<MessagePort> ElectronMessagingDelegate::CreateReceiverForTab(
    base::WeakPtr<MessagePort::ChannelDelegate> channel_delegate,
    const std::string& extension_id,
    const PortId& receiver_port_id,
    content::WebContents* receiver_contents,
    int receiver_frame_id,
    const std::string& receiver_document_id) {
  // Frame ID -1 is every frame in the tab.
  bool include_child_frames =
      receiver_frame_id == -1 && receiver_document_id.empty();

  content::RenderFrameHost* receiver_rfh = nullptr;
  if (include_child_frames) {
    // The target is the active outermost main frame of the WebContents.
    receiver_rfh = receiver_contents->GetPrimaryMainFrame();
  } else if (!receiver_document_id.empty()) {
    ExtensionApiFrameIdMap::DocumentId document_id =
        ExtensionApiFrameIdMap::DocumentIdFromString(receiver_document_id);

    // Return early for invalid documentIds.
    if (!document_id)
      return nullptr;

    receiver_rfh =
        ExtensionApiFrameIdMap::Get()->GetRenderFrameHostByDocumentId(
            document_id);

    // If both |document_id| and |receiver_frame_id| are provided they
    // should find the same RenderFrameHost, if not return early.
    if (receiver_frame_id != -1 &&
        ExtensionApiFrameIdMap::GetRenderFrameHostById(
            receiver_contents, receiver_frame_id) != receiver_rfh) {
      return nullptr;
    }
  } else {
    DCHECK_GT(receiver_frame_id, -1);
    receiver_rfh = ExtensionApiFrameIdMap::GetRenderFrameHostById(
        receiver_contents, receiver_frame_id);
  }
  if (!receiver_rfh)
    return nullptr;

  return std::make_unique<ExtensionMessagePort>(
      channel_delegate, receiver_port_id, extension_id, receiver_rfh,
      include_child_frames);
}

std::unique_ptr<MessagePort>
ElectronMessagingDelegate::CreateReceiverForNativeApp(
    content::BrowserContext* browser_context,
    base::WeakPtr<MessagePort::ChannelDelegate> channel_delegate,
    content::RenderFrameHost* source,
    const std::string& extension_id,
    const PortId& receiver_port_id,
    const std::string& native_app_name,
    bool allow_user_level,
    std::string* error_out) {
  return nullptr;
}

void ElectronMessagingDelegate::QueryIncognitoConnectability(
    content::BrowserContext* context,
    const Extension* target_extension,
    content::WebContents* source_contents,
    const GURL& source_url,
    base::OnceCallback<void(bool)> callback) {
  DCHECK(context->IsOffTheRecord());
  std::move(callback).Run(false);
}

}  // namespace extensions
