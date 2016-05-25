// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/web_view_manager.h"

#include "atom/browser/atom_browser_context.h"
#include "content/public/browser/render_process_host.h"

namespace atom {

WebViewManager::WebViewManager() {
}

WebViewManager::~WebViewManager() {
}

void WebViewManager::AddGuest(int guest_instance_id,
                              int element_instance_id,
                              content::WebContents* embedder,
                              content::WebContents* web_contents) {
  web_contents_embedder_map_[guest_instance_id] = { web_contents, embedder };

  // Map the element in embedder to guest.
  int owner_process_id = embedder->GetRenderProcessHost()->GetID();
  ElementInstanceKey key(owner_process_id, element_instance_id);
  element_instance_id_to_guest_map_[key] = guest_instance_id;
}

void WebViewManager::RemoveGuest(int guest_instance_id) {
  if (!ContainsKey(web_contents_embedder_map_, guest_instance_id))
    return;

  web_contents_embedder_map_.erase(guest_instance_id);

  // Remove the record of element in embedder too.
  for (const auto& element : element_instance_id_to_guest_map_)
    if (element.second == guest_instance_id) {
      element_instance_id_to_guest_map_.erase(element.first);
      break;
    }
}

content::WebContents* WebViewManager::GetEmbedder(int guest_instance_id) {
  if (ContainsKey(web_contents_embedder_map_, guest_instance_id))
    return web_contents_embedder_map_[guest_instance_id].embedder;
  else
    return nullptr;
}

content::WebContents* WebViewManager::GetGuestByInstanceID(
    int owner_process_id,
    int element_instance_id) {
  ElementInstanceKey key(owner_process_id, element_instance_id);
  if (!ContainsKey(element_instance_id_to_guest_map_, key))
    return nullptr;

  int guest_instance_id = element_instance_id_to_guest_map_[key];
  if (ContainsKey(web_contents_embedder_map_, guest_instance_id))
    return web_contents_embedder_map_[guest_instance_id].web_contents;
  else
    return nullptr;
}

bool WebViewManager::ForEachGuest(content::WebContents* embedder_web_contents,
                                  const GuestCallback& callback) {
  for (auto& item : web_contents_embedder_map_)
    if (item.second.embedder == embedder_web_contents &&
        callback.Run(item.second.web_contents))
      return true;
  return false;
}

// static
WebViewManager* WebViewManager::GetWebViewManager(
    content::WebContents* web_contents) {
  auto context = web_contents->GetBrowserContext();
  if (context) {
    auto manager = context->GetGuestManager();
    return static_cast<atom::WebViewManager*>(manager);
  } else {
    return nullptr;
  }
}

}  // namespace atom
