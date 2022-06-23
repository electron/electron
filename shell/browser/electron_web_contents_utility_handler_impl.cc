// Copyright (c) 2022 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/electron_web_contents_utility_handler_impl.h"

#include <utility>

#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "mojo/public/cpp/bindings/self_owned_receiver.h"

namespace electron {
// static
void ElectronWebContentsUtilityHandlerImpl::BindElectronWebContentsUtility(
    mojo::PendingAssociatedReceiver<mojom::ElectronWebContentsUtility> receiver,
    content::RenderFrameHost* rfh) {
  auto* web_contents = content::WebContents::FromRenderFrameHost(rfh);
  DCHECK(web_contents);
  CreateForWebContents(web_contents);
  auto* handler = FromWebContents(web_contents);
  handler->receivers_.Bind(rfh, std::move(receiver));
}

ElectronWebContentsUtilityHandlerImpl::ElectronWebContentsUtilityHandlerImpl(
    content::WebContents* web_contents)
    : content::WebContentsUserData<ElectronWebContentsUtilityHandlerImpl>(
          *web_contents),
      receivers_(web_contents, this) {}

ElectronWebContentsUtilityHandlerImpl::
    ~ElectronWebContentsUtilityHandlerImpl() = default;

void ElectronWebContentsUtilityHandlerImpl::OnFirstNonEmptyLayout() {
  api::WebContents* api_web_contents =
      api::WebContents::From(&GetWebContents());
  if (api_web_contents) {
    api_web_contents->OnFirstNonEmptyLayout(receivers_.GetCurrentTargetFrame());
  }
}

void ElectronWebContentsUtilityHandlerImpl::UpdateDraggableRegions(
    std::vector<mojom::DraggableRegionPtr> regions) {
  api::WebContents* api_web_contents =
      api::WebContents::From(&GetWebContents());
  if (api_web_contents) {
    api_web_contents->UpdateDraggableRegions(std::move(regions));
  }
}

void ElectronWebContentsUtilityHandlerImpl::SetTemporaryZoomLevel(
    double level) {
  api::WebContents* api_web_contents =
      api::WebContents::From(&GetWebContents());
  if (api_web_contents) {
    api_web_contents->SetTemporaryZoomLevel(level);
  }
}

void ElectronWebContentsUtilityHandlerImpl::DoGetZoomLevel(
    DoGetZoomLevelCallback callback) {
  api::WebContents* api_web_contents =
      api::WebContents::From(&GetWebContents());
  if (api_web_contents) {
    api_web_contents->DoGetZoomLevel(std::move(callback));
  }
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(ElectronWebContentsUtilityHandlerImpl);
}  // namespace electron
