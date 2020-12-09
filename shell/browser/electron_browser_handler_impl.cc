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
      render_frame_id_(frame_host->GetRoutingID()),
      weak_factory_(this) {
  content::WebContents* web_contents =
      content::WebContents::FromRenderFrameHost(frame_host);
  content::WebContentsObserver::Observe(web_contents);
}

ElectronBrowserHandlerImpl::~ElectronBrowserHandlerImpl() = default;

void ElectronBrowserHandlerImpl::BindTo(
    mojo::PendingReceiver<mojom::ElectronBrowser> receiver) {
  // Note: BindTo might be called for multiple times.
  if (receiver_.is_bound())
    receiver_.reset();

  receiver_.Bind(std::move(receiver));
  receiver_.set_disconnect_handler(base::BindOnce(
      &ElectronBrowserHandlerImpl::OnConnectionError, GetWeakPtr()));
}

void ElectronBrowserHandlerImpl::WebContentsDestroyed() {
  delete this;
}

void ElectronBrowserHandlerImpl::OnConnectionError() {
  if (receiver_.is_bound())
    receiver_.reset();
}

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
                                           int32_t web_contents_id,
                                           const std::string& channel,
                                           blink::CloneableMessage arguments) {
  api::WebContents* api_web_contents = GetAPIWebContents();
  if (api_web_contents) {
    api_web_contents->MessageTo(internal, web_contents_id, channel,
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
  auto* browser_handler = new ElectronBrowserHandlerImpl(frame_host);
  browser_handler->BindTo(std::move(receiver));
}
}  // namespace electron
