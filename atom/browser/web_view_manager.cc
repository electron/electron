// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/web_view_manager.h"

#include "atom/browser/atom_browser_context.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/web_contents.h"

namespace atom {

namespace {

WebViewManager* GetManagerFromProcess(content::RenderProcessHost* process) {
  if (!process)
    return nullptr;
  auto context = process->GetBrowserContext();
  if (!context)
    return nullptr;
  return static_cast<WebViewManager*>(context->GetGuestManager());
}

}  // namespace

// static
bool WebViewManager::GetInfoForProcess(content::RenderProcessHost* process,
                                       WebViewInfo* info) {
  auto manager = GetManagerFromProcess(process);
  if (!manager)
    return false;
  return manager->GetInfo(process->GetID(), info);
}

// static
void WebViewManager::UpdateGuestProcessID(
    content::RenderProcessHost* old_process,
    content::RenderProcessHost* new_process) {
  auto manager = GetManagerFromProcess(old_process);
  if (manager) {
    base::AutoLock auto_lock(manager->lock_);
    int old_id = old_process->GetID();
    int new_id = new_process->GetID();
    if (!ContainsKey(manager->webview_info_map_, old_id))
      return;
    manager->webview_info_map_[new_id] = manager->webview_info_map_[old_id];
    manager->webview_info_map_.erase(old_id);
  }
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
  int owner_process_id = embedder->GetRenderProcessHost()->GetID();
  ElementInstanceKey key(owner_process_id, element_instance_id);
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
    int owner_process_id,
    int element_instance_id) {
  ElementInstanceKey key(owner_process_id, element_instance_id);
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
