// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_WEB_VIEW_GUEST_DELEGATE_H_
#define ATOM_BROWSER_WEB_VIEW_GUEST_DELEGATE_H_

#include "atom/browser/web_contents_zoom_controller.h"
#include "content/public/browser/browser_plugin_guest_delegate.h"

namespace atom {

namespace api {
class WebContents;
}

class WebViewGuestDelegate : public content::BrowserPluginGuestDelegate,
                             public WebContentsZoomController::Observer {
 public:
  WebViewGuestDelegate(content::WebContents* embedder,
                       api::WebContents* api_web_contents);
  ~WebViewGuestDelegate() override;

  // Attach to the iframe.
  void AttachToIframe(content::WebContents* embedder_web_contents,
                      int embedder_frame_id);

 protected:
  // content::BrowserPluginGuestDelegate:
  void DidDetach() final;
  content::WebContents* GetOwnerWebContents() const final;
  content::RenderWidgetHost* GetOwnerRenderWidgetHost() final;
  content::SiteInstance* GetOwnerSiteInstance() final;
  content::WebContents* CreateNewGuestWindow(
      const content::WebContents::CreateParams& create_params) final;

  // WebContentsZoomController::Observer:
  void OnZoomLevelChanged(content::WebContents* web_contents,
                          double level,
                          bool is_temporary) override;
  void OnZoomControllerWebContentsDestroyed() override;

 private:
  void ResetZoomController();

  // The WebContents that attaches this guest view.
  content::WebContents* embedder_web_contents_ = nullptr;

  // The zoom controller of the embedder that is used
  // to subscribe for zoom changes.
  WebContentsZoomController* embedder_zoom_controller_ = nullptr;

  api::WebContents* api_web_contents_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(WebViewGuestDelegate);
};

}  // namespace atom

#endif  // ATOM_BROWSER_WEB_VIEW_GUEST_DELEGATE_H_
