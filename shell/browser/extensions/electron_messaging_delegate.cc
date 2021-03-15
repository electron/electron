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
#include "extensions/browser/api/messaging/extension_message_port.h"
#include "extensions/browser/api/messaging/native_message_host.h"
#include "extensions/browser/extension_api_frame_id_map.h"
#include "extensions/browser/pref_names.h"
#include "extensions/common/api/messaging/port_id.h"
#include "extensions/common/extension.h"
#include "shell/browser/api/electron_api_session.h"
#include "shell/browser/api/electron_api_web_contents.h"
#include "shell/browser/extensions/extension_tab_util.h"
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

std::unique_ptr<base::DictionaryValue>
ElectronMessagingDelegate::MaybeGetTabInfo(content::WebContents* web_contents) {
  // Add info about the opener's tab (if it was a tab).
  if (web_contents && ExtensionTabUtil::GetTabId(web_contents) >= 0) {
    // Only the tab id is useful to platform apps for internal use. The
    // unnecessary bits will be stripped out in
    // MessagingBindings::DispatchOnConnect().
    // Note: We don't bother scrubbing the tab object, because this is only
    // reached as a result of a tab (or content script) messaging the extension.
    // We need the extension to see the sender so that it can validate if it
    // trusts it or not.
    // TODO(tjudkins): Adjust scrubbing behavior in this situation to not scrub
    // the last committed URL, but do scrub the pending URL based on
    // permissions.
    auto* contents = electron::api::WebContents::From(web_contents);
    if (!contents)
      return nullptr;

    auto* session = electron::api::Session::FromBrowserContext(
        web_contents->GetBrowserContext());
    if (!session)
      return nullptr;

    auto tab = session->GetExtensionTabDetails(contents);
    if (!tab)
      return nullptr;

    ExtensionTabUtil::ScrubTabBehavior scrub_tab_behavior = {
        ExtensionTabUtil::kDontScrubTab, ExtensionTabUtil::kDontScrubTab};
    return ExtensionTabUtil::CreateTabObject(web_contents, *tab.get(),
                                             scrub_tab_behavior, nullptr, -1)
        ->ToValue();
  }
  return nullptr;
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
    int receiver_frame_id) {
  // Frame ID -1 is every frame in the tab.
  bool include_child_frames = receiver_frame_id == -1;
  content::RenderFrameHost* receiver_rfh =
      include_child_frames ? receiver_contents->GetMainFrame()
                           : ExtensionApiFrameIdMap::GetRenderFrameHostById(
                                 receiver_contents, receiver_frame_id);
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
