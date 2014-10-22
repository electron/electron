// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/web_view/web_view_manager.h"

#include "base/logging.h"

namespace atom {

WebViewManager::WebViewManager(content::BrowserContext* context) {
}

WebViewManager::~WebViewManager() {
}

void WebViewManager::MaybeGetGuestByInstanceIDOrKill(
    int guest_instance_id,
    int embedder_render_process_id,
    const GuestByInstanceIDCallback& callback) {
  LOG(ERROR) << "MaybeGetGuestByInstanceIDOrKill";
  callback.Run(nullptr);
}

bool WebViewManager::ForEachGuest(content::WebContents* embedder_web_contents,
                                  const GuestCallback& callback) {
  LOG(ERROR) << "ForEachGuest";
  return false;
}

}  // namespace atom
