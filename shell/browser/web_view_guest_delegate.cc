// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/web_view_guest_delegate.h"

#include <memory>

#include "content/browser/web_contents/web_contents_impl.h"  // nogncheck
#include "content/browser/web_contents/web_contents_view.h"  // nogncheck
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_widget_host.h"
#include "content/public/browser/render_widget_host_view.h"
#include "shell/browser/api/electron_api_web_contents.h"
#include "third_party/blink/public/common/page/page_zoom.h"

namespace electron {

WebViewGuestDelegate::WebViewGuestDelegate(content::WebContents* embedder,
                                           api::WebContents* api_web_contents)
    : embedder_web_contents_(embedder ? embedder->GetWeakPtr() : nullptr),
      api_web_contents_(api_web_contents) {}

WebViewGuestDelegate::~WebViewGuestDelegate() {
  ResetZoomController();
}

void WebViewGuestDelegate::AttachToIframe(
    content::WebContents* embedder_web_contents,
    blink::LocalFrameToken& embedder_frame_token) {
  embedder_web_contents_ = embedder_web_contents->GetWeakPtr();

  int embedder_process_id = embedder_web_contents->GetPrimaryMainFrame()
                                ->GetProcess()
                                ->GetDeprecatedID();
  auto* embedder_frame = content::RenderFrameHost::FromFrameToken(
      content::GlobalRenderFrameHostToken(embedder_process_id,
                                          embedder_frame_token));
  DCHECK_EQ(embedder_web_contents_.get(),
            content::WebContents::FromRenderFrameHost(embedder_frame));

  content::WebContents* guest_web_contents = api_web_contents_->web_contents();

  // Attach this inner WebContents |guest_web_contents| to the outer
  // WebContents |embedder_web_contents|. The outer WebContents's
  // frame |embedder_frame| hosts the inner WebContents.
  embedder_web_contents->AttachInnerWebContents(
      base::WrapUnique<content::WebContents>(guest_web_contents),
      embedder_frame,
      /*is_full_page=*/false);

  ResetZoomController();

  content::WebContents* owner = embedder_web_contents_.get();
  if (!owner)
    return;
  embedder_zoom_controller_ = WebContentsZoomController::FromWebContents(owner);
  if (!embedder_zoom_controller_)
    return;
  embedder_zoom_controller_->AddObserver(this);
  auto* zoom_controller = api_web_contents_->GetZoomController();
  zoom_controller->SetEmbedderZoomController(embedder_zoom_controller_);

  api_web_contents_->Emit("did-attach");
}

void WebViewGuestDelegate::WillDestroy() {
  ResetZoomController();
  embedder_web_contents_.reset();
}

content::WebContents* WebViewGuestDelegate::GetOwnerWebContents() {
  return embedder_web_contents_.get();
}

void WebViewGuestDelegate::OnZoomChanged(
    const WebContentsZoomController::ZoomChangedEventData& data) {
  if (!api_web_contents_ || api_web_contents_->IsDestroyed())
    return;
  if (data.web_contents == GetOwnerWebContents()) {
    auto* zoom_controller = api_web_contents_->GetZoomController();
    if (data.temporary) {
      zoom_controller->SetTemporaryZoomLevel(data.new_zoom_level);
    } else {
      if (blink::ZoomValuesEqual(data.new_zoom_level,
                                 zoom_controller->GetZoomLevel()))
        return;
      zoom_controller->SetZoomLevel(data.new_zoom_level);
    }
    // Change the default zoom factor to match the embedders' new zoom level.
    double zoom_factor = blink::ZoomLevelToZoomFactor(data.new_zoom_level);
    zoom_controller->SetDefaultZoomFactor(zoom_factor);
  }
}

void WebViewGuestDelegate::OnZoomControllerDestroyed(
    WebContentsZoomController* zoom_controller) {
  ResetZoomController();
}

void WebViewGuestDelegate::ResetZoomController() {
  if (embedder_zoom_controller_) {
    embedder_zoom_controller_->RemoveObserver(this);
    embedder_zoom_controller_ = nullptr;
  }
}

std::unique_ptr<content::WebContents>
WebViewGuestDelegate::CreateNewGuestWindow(
    const content::WebContents::CreateParams& create_params) {
  content::WebContents* embedder = embedder_web_contents_.get();
  if (!embedder)
    return nullptr;

  // Code below mirrors what content::WebContentsImpl::CreateNewWindow
  // does for non-guest sources
  content::WebContents::CreateParams guest_params(create_params);
  guest_params.context = embedder->GetNativeView();
  std::unique_ptr<content::WebContents> guest_contents =
      content::WebContents::Create(guest_params);
  if (!create_params.opener_suppressed) {
    auto* guest_contents_impl =
        static_cast<content::WebContentsImpl*>(guest_contents.release());
    auto* new_guest_view = guest_contents_impl->GetView();

    content::RenderWidgetHostView* widget_view = nullptr;
    if (new_guest_view) {
      widget_view = new_guest_view->CreateViewForWidget(
          guest_contents_impl->GetRenderViewHost()->GetWidget());
    }

    if (widget_view && !create_params.initially_hidden)
      guest_contents_impl->WasShown();
    return base::WrapUnique(
        static_cast<content::WebContentsImpl*>(guest_contents_impl));
  }
  return guest_contents;
}

base::WeakPtr<content::BrowserPluginGuestDelegate>
WebViewGuestDelegate::GetGuestDelegateWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

}  // namespace electron
