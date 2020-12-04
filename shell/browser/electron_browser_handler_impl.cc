// Copyright (c) 2019 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/electron_browser_handler_impl.h"

#include <utility>

#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "mojo/public/cpp/bindings/self_owned_receiver.h"

namespace electron {
ElectronBrowserHandlerImpl::ElectronBrowserHandlerImpl(
    content::RenderFrameHost* frame_host)
    : render_process_id_(frame_host->GetProcess()->GetID()),
      render_frame_id_(frame_host->GetRoutingID()) {}

ElectronBrowserHandlerImpl::~ElectronBrowserHandlerImpl() = default;

void ElectronBrowserHandlerImpl::Message(bool internal,
                                         const std::string& channel,
                                         blink::CloneableMessage arguments) {
  content::RenderFrameHost* render_frame_host =
      content::RenderFrameHost::FromID(render_process_id_, render_frame_id_);
  api::WebContents* api_web_contents = api::WebContents::From(
      content::WebContents::FromRenderFrameHost(render_frame_host));
  if (api_web_contents) {
    api_web_contents->Message(internal, channel, std::move(arguments),
                              render_frame_host);
  }
}
void ElectronBrowserHandlerImpl::Invoke(bool internal,
                                        const std::string& channel,
                                        blink::CloneableMessage arguments,
                                        InvokeCallback callback) {
  content::RenderFrameHost* render_frame_host =
      content::RenderFrameHost::FromID(render_process_id_, render_frame_id_);
  api::WebContents* api_web_contents = api::WebContents::From(
      content::WebContents::FromRenderFrameHost(render_frame_host));
  if (api_web_contents) {
    api_web_contents->Invoke(internal, channel, std::move(arguments),
                             std::move(callback), render_frame_host);
  }
}

void ElectronBrowserHandlerImpl::OnFirstNonEmptyLayout() {
  content::RenderFrameHost* render_frame_host =
      content::RenderFrameHost::FromID(render_process_id_, render_frame_id_);
  api::WebContents* api_web_contents = api::WebContents::From(
      content::WebContents::FromRenderFrameHost(render_frame_host));
  if (api_web_contents) {
    api_web_contents->OnFirstNonEmptyLayout(render_frame_host);
  }
}

void ElectronBrowserHandlerImpl::ReceivePostMessage(
    const std::string& channel,
    blink::TransferableMessage message) {
  content::RenderFrameHost* render_frame_host =
      content::RenderFrameHost::FromID(render_process_id_, render_frame_id_);
  api::WebContents* api_web_contents = api::WebContents::From(
      content::WebContents::FromRenderFrameHost(render_frame_host));
  if (api_web_contents) {
    api_web_contents->ReceivePostMessage(channel, std::move(message),
                                         render_frame_host);
  }
}

void ElectronBrowserHandlerImpl::MessageSync(bool internal,
                                             const std::string& channel,
                                             blink::CloneableMessage arguments,
                                             MessageSyncCallback callback) {
  content::RenderFrameHost* render_frame_host =
      content::RenderFrameHost::FromID(render_process_id_, render_frame_id_);
  api::WebContents* api_web_contents = api::WebContents::From(
      content::WebContents::FromRenderFrameHost(render_frame_host));
  if (api_web_contents) {
    api_web_contents->MessageSync(internal, channel, std::move(arguments),
                                  std::move(callback), render_frame_host);
  }
}

void ElectronBrowserHandlerImpl::MessageTo(bool internal,
                                           bool send_to_all,
                                           int32_t web_contents_id,
                                           const std::string& channel,
                                           blink::CloneableMessage arguments) {
  api::WebContents* api_web_contents = GetAPIWebContents();
  if (api_web_contents) {
    api_web_contents->MessageTo(internal, send_to_all, web_contents_id, channel,
                                std::move(arguments));
  }
}

void ElectronBrowserHandlerImpl::MessageHost(
    const std::string& channel,
    blink::CloneableMessage arguments) {
  content::RenderFrameHost* render_frame_host =
      content::RenderFrameHost::FromID(render_process_id_, render_frame_id_);
  api::WebContents* api_web_contents = api::WebContents::From(
      content::WebContents::FromRenderFrameHost(render_frame_host));
  if (api_web_contents) {
    api_web_contents->MessageHost(channel, std::move(arguments),
                                  render_frame_host);
  }
}

void ElectronBrowserHandlerImpl::UpdateDraggableRegions(
    std::vector<mojom::DraggableRegionPtr> regions) {
  api::WebContents* api_web_contents = GetAPIWebContents();
  if (api_web_contents) {
    api_web_contents->UpdateDraggableRegions(std::move(regions));
  }
}

void ElectronBrowserHandlerImpl::SetTemporaryZoomLevel(double level) {
  api::WebContents* api_web_contents = GetAPIWebContents();
  if (api_web_contents) {
    api_web_contents->SetTemporaryZoomLevel(level);
  }
}

void ElectronBrowserHandlerImpl::DoGetZoomLevel(
    DoGetZoomLevelCallback callback) {
  api::WebContents* api_web_contents = GetAPIWebContents();
  if (api_web_contents) {
    api_web_contents->DoGetZoomLevel(std::move(callback));
  }
}

api::WebContents* ElectronBrowserHandlerImpl::GetAPIWebContents() {
  content::RenderFrameHost* render_frame_host =
      content::RenderFrameHost::FromID(render_process_id_, render_frame_id_);
  return api::WebContents::From(
      content::WebContents::FromRenderFrameHost(render_frame_host));
}

// static
void ElectronBrowserHandlerImpl::Create(
    content::RenderFrameHost* frame_host,
    mojo::PendingReceiver<mojom::ElectronBrowser> receiver) {
  mojo::MakeSelfOwnedReceiver(
      base::WrapUnique(new ElectronBrowserHandlerImpl(frame_host)),
      std::move(receiver));
}
}  // namespace electron
