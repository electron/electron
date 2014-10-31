// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/web_view/web_view_renderer_state.h"

#include "content/public/browser/browser_thread.h"

namespace atom {

// static
WebViewRendererState* WebViewRendererState::GetInstance() {
  return Singleton<WebViewRendererState>::get();
}

WebViewRendererState::WebViewRendererState() {
}

WebViewRendererState::~WebViewRendererState() {
}

bool WebViewRendererState::IsGuest(int render_process_id) {
  return webview_info_map_.find(render_process_id) !=
         webview_info_map_.end();
}

void WebViewRendererState::AddGuest(int guest_process_id,
                                    const WebViewInfo& webview_info) {
  webview_info_map_[guest_process_id] = webview_info;
}

void WebViewRendererState::RemoveGuest(int guest_process_id) {
  webview_info_map_.erase(guest_process_id);
}

bool WebViewRendererState::GetInfo(int guest_process_id,
                                   WebViewInfo* webview_info) {
  WebViewInfoMap::iterator iter = webview_info_map_.find(guest_process_id);
  if (iter != webview_info_map_.end()) {
    *webview_info = iter->second;
    return true;
  }
  return false;
}

}  // namespace atom
