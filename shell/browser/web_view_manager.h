// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_WEB_VIEW_MANAGER_H_
#define ELECTRON_SHELL_BROWSER_WEB_VIEW_MANAGER_H_

#include <map>

#include "base/memory/raw_ptr.h"
#include "content/public/browser/browser_plugin_guest_manager.h"

namespace electron {

class WebViewManager : public content::BrowserPluginGuestManager {
 public:
  WebViewManager();
  ~WebViewManager() override;

  // disable copy
  WebViewManager(const WebViewManager&) = delete;
  WebViewManager& operator=(const WebViewManager&) = delete;

  void AddGuest(int guest_instance_id,
                content::WebContents* embedder,
                content::WebContents* web_contents);
  void RemoveGuest(int guest_instance_id);

  static WebViewManager* GetWebViewManager(content::WebContents* web_contents);

  // content::BrowserPluginGuestManager:
  bool ForEachGuest(content::WebContents* embedder,
                    base::FunctionRef<bool(content::WebContents*)> fn) override;

 private:
  struct WebContentsWithEmbedder {
    raw_ptr<content::WebContents> web_contents;
    raw_ptr<content::WebContents> embedder;
  };
  // guest_instance_id => (web_contents, embedder)
  std::map<int, WebContentsWithEmbedder> web_contents_embedder_map_;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_WEB_VIEW_MANAGER_H_
