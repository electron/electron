// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_WEB_VIEW_WEB_VIEW_MANAGER_H_
#define ATOM_BROWSER_WEB_VIEW_WEB_VIEW_MANAGER_H_

#include <map>

#include "content/public/browser/browser_plugin_guest_manager.h"
#include "url/gurl.h"

namespace content {
class BrowserContext;
}

namespace atom {

class WebViewManager : public content::BrowserPluginGuestManager {
 public:
  explicit WebViewManager(content::BrowserContext* context);
  virtual ~WebViewManager();

  struct WebViewOptions {
    bool node_integration;
    bool plugins;
    bool disable_web_security;
    GURL preload_url;
  };

  void AddGuest(int guest_instance_id,
                int element_instance_id,
                content::WebContents* embedder,
                content::WebContents* web_contents,
                const WebViewOptions& options);
  void RemoveGuest(int guest_instance_id);

 protected:
  // content::BrowserPluginGuestManager:
  content::WebContents* GetGuestByInstanceID(
      content::WebContents* embedder_web_contents,
      int element_instance_id) override;
  bool ForEachGuest(content::WebContents* embedder,
                    const GuestCallback& callback) override;

 private:
  struct WebContentsWithEmbedder {
    content::WebContents* web_contents;  // Weak ref.
    content::WebContents* embedder;
  };
  std::map<int, WebContentsWithEmbedder> web_contents_map_;

  struct ElementInstanceKey {
    content::WebContents* owner_web_contents;
    int element_instance_id;

    ElementInstanceKey()
        : owner_web_contents(nullptr),
          element_instance_id(0) {}

    ElementInstanceKey(content::WebContents* owner_web_contents,
                       int element_instance_id)
        : owner_web_contents(owner_web_contents),
          element_instance_id(element_instance_id) {}

    bool operator<(const ElementInstanceKey& other) const {
      if (owner_web_contents != other.owner_web_contents)
        return owner_web_contents < other.owner_web_contents;
      return element_instance_id < other.element_instance_id;
    }

    bool operator==(const ElementInstanceKey& other) const {
      return (owner_web_contents == other.owner_web_contents) &&
          (element_instance_id == other.element_instance_id);
    }
  };
  std::map<ElementInstanceKey, int> element_instance_id_to_guest_map_;

  DISALLOW_COPY_AND_ASSIGN(WebViewManager);
};

}  // namespace atom

#endif  // ATOM_BROWSER_WEB_VIEW_WEB_VIEW_MANAGER_H_
