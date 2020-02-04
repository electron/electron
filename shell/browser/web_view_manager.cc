// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/web_view_manager.h"

#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/web_contents.h"
#include "shell/browser/electron_browser_context.h"

namespace electron {

WebViewManager::WebViewManager() = default;

WebViewManager::~WebViewManager() = default;

void WebViewManager::AddGuest(int guest_instance_id,
                              int element_instance_id,
                              content::WebContents* embedder,
                              content::WebContents* web_contents) {
  web_contents_embedder_map_[guest_instance_id] = {web_contents, embedder};

  // Map the element in embedder to guest.
  int owner_process_id = embedder->GetMainFrame()->GetProcess()->GetID();
  ElementInstanceKey key(owner_process_id, element_instance_id);
  element_instance_id_to_guest_map_[key] = guest_instance_id;
}

void WebViewManager::RemoveGuest(int guest_instance_id) {
  if (web_contents_embedder_map_.erase(guest_instance_id) == 0)
    return;

  // Remove the record of element in embedder too.
  for (const auto& element : element_instance_id_to_guest_map_)
    if (element.second == guest_instance_id) {
      element_instance_id_to_guest_map_.erase(element.first);
      break;
    }
}

content::WebContents* WebViewManager::GetEmbedder(int guest_instance_id) {
  const auto iter = web_contents_embedder_map_.find(guest_instance_id);
  return iter == std::end(web_contents_embedder_map_) ? nullptr
                                                      : iter->second.embedder;
}

content::WebContents* WebViewManager::GetGuestByInstanceID(
    int owner_process_id,
    int element_instance_id) {
  const ElementInstanceKey key(owner_process_id, element_instance_id);
  const auto guest_iter = element_instance_id_to_guest_map_.find(key);
  if (guest_iter == std::end(element_instance_id_to_guest_map_))
    return nullptr;

  const int guest_instance_id = guest_iter->second;
  const auto iter = web_contents_embedder_map_.find(guest_instance_id);
  return iter == std::end(web_contents_embedder_map_)
             ? nullptr
             : iter->second.web_contents;
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
