// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/extensions/api/atom_extensions_api_client.h"

#include "atom/browser/extensions/atom_extension_web_contents_observer.h"
#include "atom/browser/extensions/extension_renderer_state.h"
#include "atom/browser/extensions/tab_helper.h"
#include "content/public/browser/resource_request_info.h"
#include "extensions/browser/api/web_request/web_request_event_details.h"
#include "extensions/browser/api/web_request/web_request_event_router_delegate.h"

namespace {

void ExtractExtraRequestDetailsInternal(const net::URLRequest* request,
                                        int* tab_id,
                                        int* window_id) {
  const content::ResourceRequestInfo* info =
      content::ResourceRequestInfo::ForRequest(request);
  if (!info)
    return;

  ExtensionRendererState::GetInstance()->GetTabAndWindowId(info, tab_id,
                                                           window_id);
}

}  // namespace

namespace extensions {

class AtomExtensionWebRequestEventRouterDelegate :
    public WebRequestEventRouterDelegate {
 public:
  AtomExtensionWebRequestEventRouterDelegate() {}
  ~AtomExtensionWebRequestEventRouterDelegate() {}

  // Looks up the tab and window ID for a given request.
  // Called on the IO thread.
  void ExtractExtraRequestDetails(const net::URLRequest* request,
                                  WebRequestEventDetails* out) override {
    int tab_id = -1;
    int window_id = -1;
    ExtractExtraRequestDetailsInternal(request, &tab_id, &window_id);
    out->SetInteger(keys::kTabIdKey, tab_id);
  };

  // Called to check extra parameters (e.g., tab_id, windown_id) when filtering
  // event listeners.
  bool OnGetMatchingListenersImplCheck(
      int filter_tab_id,
      int filter_window_id,
      const net::URLRequest* request) override {
    int tab_id = -1;
    int window_id = -1;
    ExtractExtraRequestDetailsInternal(request, &tab_id, &window_id);
    if (filter_tab_id != -1 && tab_id != filter_tab_id)
      return true;
    return (filter_window_id != -1 && window_id != filter_window_id);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(AtomExtensionWebRequestEventRouterDelegate);
};

AtomExtensionsAPIClient::AtomExtensionsAPIClient() {
}

void AtomExtensionsAPIClient::AttachWebContentsHelpers(
    content::WebContents* web_contents) const {
  extensions::TabHelper::CreateForWebContents(web_contents);
  AtomExtensionWebContentsObserver::CreateForWebContents(web_contents);
}

WebViewGuestDelegate* AtomExtensionsAPIClient::CreateWebViewGuestDelegate(
    WebViewGuest* web_view_guest) const {
  return ExtensionsAPIClient::CreateWebViewGuestDelegate(web_view_guest);
}

WebRequestEventRouterDelegate*
AtomExtensionsAPIClient::CreateWebRequestEventRouterDelegate() const {
  return new AtomExtensionWebRequestEventRouterDelegate();
}

}  // namespace extensions
