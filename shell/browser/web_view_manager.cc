// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/web_view_manager.h"

#include "content/public/browser/web_contents.h"
#include "shell/browser/electron_browser_context.h"

namespace electron {

WebViewManager::WebViewManager() = default;

WebViewManager::~WebViewManager() = default;

void WebViewManager::AddGuest(int guest_instance_id,
                              content::WebContents* embedder,
                              content::WebContents* web_contents) {
  web_contents_embedder_map_[guest_instance_id] = {web_contents, embedder};
}

void WebViewManager::RemoveGuest(int guest_instance_id) {
  web_contents_embedder_map_.erase(guest_instance_id);
}

bool WebViewManager::ForEachGuest(content::WebContents* embedder_web_contents,
                                  const GuestCallback& callback) {
  for (auto& item : web_contents_embedder_map_) {
    if (item.second.embedder != embedder_web_contents)
      continue;

    auto* guest_web_contents = item.second.web_contents;
    if (guest_web_contents && callback.Run(guest_web_contents))
      return true;
  }
  return false;
}

// static
WebViewManager* WebViewManager::GetWebViewManager(
    content::WebContents* web_contents) {
  auto* context = web_contents->GetBrowserContext();
  if (context) {
    auto* manager = context->GetGuestManager();
    return static_cast<WebViewManager*>(manager);
  } else {
    return nullptr;
  }
}

}  // namespace electron
