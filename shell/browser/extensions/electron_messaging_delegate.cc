// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/browser/extensions/electron_messaging_delegate.h"

#include <memory>
#include <utility>

#include "base/functional/callback.h"
#include "base/values.h"
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
#include "ui/gfx/native_ui_types.h"
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

std::optional<base::DictValue> ElectronMessagingDelegate::MaybeGetTabInfo(
    content::WebContents* web_contents) {
  if (web_contents) {
    auto* api_contents = electron::api::WebContents::From(web_contents);
    if (api_contents) {
      api::tabs::Tab tab;
      tab.id = api_contents->ID();
      tab.url = api_contents->GetURL().spec();
      tab.title = base::UTF16ToUTF8(api_contents->GetTitle());
      tab.audible = api_contents->IsCurrentlyAudible();
      return tab.ToValue();
    }
  }
  return std::nullopt;
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
