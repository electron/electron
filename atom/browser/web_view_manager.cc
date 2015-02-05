// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/web_view_manager.h"

#include "atom/browser/atom_browser_context.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/web_contents.h"

namespace atom {

// static
bool WebViewManager::GetInfoForProcess(content::RenderProcessHost* process,
                                       WebViewInfo* info) {
  if (!process)
    return false;
  auto context = process->GetBrowserContext();
  if (!context)
    return false;
  auto manager = context->GetGuestManager();
  if (!manager)
    return false;
  return static_cast<WebViewManager*>(manager)->GetInfo(process->GetID(), info);
}

WebViewManager::WebViewManager(content::BrowserContext* context) {
}

WebViewManager::~WebViewManager() {
}

void WebViewManager::AddGuest(int guest_instance_id,
                              int element_instance_id,
                              content::WebContents* embedder,
                              content::WebContents* web_contents,
                              const WebViewInfo& info) {
  base::AutoLock auto_lock(lock_);
  web_contents_embdder_map_[guest_instance_id] = { web_contents, embedder };

  int guest_process_id = web_contents->GetRenderProcessHost()->GetID();
  webview_info_map_[guest_process_id] = info;

  // Map the element in embedder to guest.
  ElementInstanceKey key(embedder, element_instance_id);
  element_instance_id_to_guest_map_[key] = guest_instance_id;
}

void WebViewManager::RemoveGuest(int guest_instance_id) {
  base::AutoLock auto_lock(lock_);
  if (!ContainsKey(web_contents_embdder_map_, guest_instance_id))
    return;

  auto web_contents = web_contents_embdder_map_[guest_instance_id].web_contents;
  web_contents_embdder_map_.erase(guest_instance_id);

  int guest_process_id = web_contents->GetRenderProcessHost()->GetID();
  webview_info_map_.erase(guest_process_id);

  // Remove the record of element in embedder too.
  for (const auto& element : element_instance_id_to_guest_map_)
    if (element.second == guest_instance_id) {
      element_instance_id_to_guest_map_.erase(element.first);
      break;
    }
}

bool WebViewManager::GetInfo(int guest_process_id, WebViewInfo* webview_info) {
  base::AutoLock auto_lock(lock_);
  WebViewInfoMap::iterator iter = webview_info_map_.find(guest_process_id);
  if (iter != webview_info_map_.end()) {
    *webview_info = iter->second;
    return true;
  }
  return false;
}

content::WebContents* WebViewManager::GetGuestByInstanceID(
    content::WebContents* embedder,
    int element_instance_id) {
  ElementInstanceKey key(embedder, element_instance_id);
  if (!ContainsKey(element_instance_id_to_guest_map_, key))
    return nullptr;

  int guest_instance_id = element_instance_id_to_guest_map_[key];
  if (ContainsKey(web_contents_embdder_map_, guest_instance_id))
    return web_contents_embdder_map_[guest_instance_id].web_contents;
  else
    return nullptr;
}

bool WebViewManager::ForEachGuest(content::WebContents* embedder_web_contents,
                                  const GuestCallback& callback) {
  for (auto& item : web_contents_embdder_map_)
    if (item.second.embedder == embedder_web_contents &&
        callback.Run(item.second.web_contents))
      return true;
  return false;
}

}  // namespace atom
