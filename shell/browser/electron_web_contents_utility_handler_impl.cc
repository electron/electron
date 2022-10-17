// Copyright (c) 2022 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/electron_web_contents_utility_handler_impl.h"

#include <utility>

#include "base/logging.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "mojo/public/cpp/bindings/self_owned_receiver.h"
#include "shell/browser/electron_browser_context.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/file_system_access_entry_factory.h"

namespace electron {
ElectronWebContentsUtilityHandlerImpl::ElectronWebContentsUtilityHandlerImpl(
    content::RenderFrameHost* frame_host,
    mojo::PendingAssociatedReceiver<mojom::ElectronWebContentsUtility> receiver)
    : render_process_id_(frame_host->GetProcess()->GetID()),
      render_frame_id_(frame_host->GetRoutingID()) {
  content::WebContents* web_contents =
      content::WebContents::FromRenderFrameHost(frame_host);
  DCHECK(web_contents);
  content::WebContentsObserver::Observe(web_contents);

  receiver_.Bind(std::move(receiver));
  receiver_.set_disconnect_handler(base::BindOnce(
      &ElectronWebContentsUtilityHandlerImpl::OnConnectionError, GetWeakPtr()));
}

ElectronWebContentsUtilityHandlerImpl::
    ~ElectronWebContentsUtilityHandlerImpl() = default;

void ElectronWebContentsUtilityHandlerImpl::WebContentsDestroyed() {
  delete this;
}

void ElectronWebContentsUtilityHandlerImpl::OnConnectionError() {
  delete this;
}

void ElectronWebContentsUtilityHandlerImpl::OnFirstNonEmptyLayout() {
  api::WebContents* api_web_contents = api::WebContents::From(web_contents());
  if (api_web_contents) {
    api_web_contents->OnFirstNonEmptyLayout(GetRenderFrameHost());
  }
}

void ElectronWebContentsUtilityHandlerImpl::UpdateDraggableRegions(
    std::vector<mojom::DraggableRegionPtr> regions) {
  api::WebContents* api_web_contents = api::WebContents::From(web_contents());
  if (api_web_contents) {
    api_web_contents->UpdateDraggableRegions(std::move(regions));
  }
}

void ElectronWebContentsUtilityHandlerImpl::SetTemporaryZoomLevel(
    double level) {
  api::WebContents* api_web_contents = api::WebContents::From(web_contents());
  if (api_web_contents) {
    api_web_contents->SetTemporaryZoomLevel(level);
  }
}

void ElectronWebContentsUtilityHandlerImpl::DoGetZoomLevel(
    DoGetZoomLevelCallback callback) {
  api::WebContents* api_web_contents = api::WebContents::From(web_contents());
  if (api_web_contents) {
    api_web_contents->DoGetZoomLevel(std::move(callback));
  }
}

void ElectronWebContentsUtilityHandlerImpl::ResolveFileSystemAccessTransferToken(
  mojo::PendingRemote<blink::mojom::FileSystemAccessTransferToken> token, ResolveFileSystemAccessTransferTokenCallback callback) {
  VLOG(1) << "======== enter ElectronWebContentsUtilityHandlerImpl::ResolveFileSystemAccessTransferToken";

  web_contents()
    ->GetBrowserContext()
    ->GetStoragePartition(web_contents()->GetSiteInstance())
    ->GetFileSystemAccessEntryFactory()
    ->ResolveTransferToken(std::move(token), base::BindOnce([](ResolveFileSystemAccessTransferTokenCallback callback, absl::optional<storage::FileSystemURL> fileSystemURL) {
      VLOG(1) << "======== got resolved token: " << !!fileSystemURL;
      if (fileSystemURL) {
        VLOG(1) << "======== got resolved token: " << fileSystemURL->path().AsUTF8Unsafe();
        std::move(callback).Run(fileSystemURL->path().AsUTF8Unsafe());
      } else {
        std::move(callback).Run("underlying ResolveTransferToken returned nullopt");
      }
    }, std::move(callback)));
}

content::RenderFrameHost*
ElectronWebContentsUtilityHandlerImpl::GetRenderFrameHost() {
  return content::RenderFrameHost::FromID(render_process_id_, render_frame_id_);
}

// static
void ElectronWebContentsUtilityHandlerImpl::Create(
    content::RenderFrameHost* frame_host,
    mojo::PendingAssociatedReceiver<mojom::ElectronWebContentsUtility>
        receiver) {
  new ElectronWebContentsUtilityHandlerImpl(frame_host, std::move(receiver));
}
}  // namespace electron
