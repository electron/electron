// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_WEB_VIEW_WEB_VIEW_MANAGER_H_
#define ATOM_BROWSER_WEB_VIEW_WEB_VIEW_MANAGER_H_

#include <map>

#include "content/public/browser/browser_plugin_guest_manager.h"

namespace content {
class BrowserContext;
}

namespace atom {

class WebViewManager : public content::BrowserPluginGuestManager {
 public:
  explicit WebViewManager(content::BrowserContext* context);
  virtual ~WebViewManager();

  void AddGuest(int guest_instance_id,
                content::WebContents* embedder,
                content::WebContents* web_contents);
  void RemoveGuest(int guest_instance_id);

 protected:
  // content::BrowserPluginGuestManager:
  virtual void MaybeGetGuestByInstanceIDOrKill(
      int guest_instance_id,
      int embedder_render_process_id,
      const GuestByInstanceIDCallback& callback) override;
  virtual bool ForEachGuest(content::WebContents* embedder_web_contents,
                            const GuestCallback& callback) override;

 private:
  struct WebContentsWithEmbedder {
    content::WebContents* web_contents;  // Weak ref.
    content::WebContents* embedder;
  };
  std::map<int, WebContentsWithEmbedder> web_contents_map_;

  DISALLOW_COPY_AND_ASSIGN(WebViewManager);
};

}  // namespace atom

#endif  // ATOM_BROWSER_WEB_VIEW_WEB_VIEW_MANAGER_H_
