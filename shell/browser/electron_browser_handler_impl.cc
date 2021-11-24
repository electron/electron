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
    content::RenderFrameHost* frame_host,
    mojo::PendingReceiver<mojom::ElectronBrowser> receiver)
    : render_process_id_(frame_host->GetProcess()->GetID()),
      render_frame_id_(frame_host->GetRoutingID()) {
  content::WebContents* web_contents =
      content::WebContents::FromRenderFrameHost(frame_host);
  DCHECK(web_contents);
  content::WebContentsObserver::Observe(web_contents);

  receiver_.Bind(std::move(receiver));
  receiver_.set_disconnect_handler(base::BindOnce(
      &ElectronBrowserHandlerImpl::OnConnectionError, GetWeakPtr()));
}

ElectronBrowserHandlerImpl::~ElectronBrowserHandlerImpl() = default;

void ElectronBrowserHandlerImpl::WebContentsDestroyed() {
  delete this;
}

void ElectronBrowserHandlerImpl::OnConnectionError() {
  delete this;
}

void ElectronBrowserHandlerImpl::Message(bool internal,
                                         const std::string& channel,
                                         blink::CloneableMessage arguments) {
  api::WebContents* api_web_contents = api::WebContents::From(web_contents());
  if (api_web_contents) {
    api_web_contents->Message(internal, channel, std::move(arguments),
                              GetRenderFrameHost());
  }
}
void ElectronBrowserHandlerImpl::Invoke(bool internal,
                                        const std::string& channel,
                                        blink::CloneableMessage arguments,
                                        InvokeCallback callback) {
  api::WebContents* api_web_contents = api::WebContents::From(web_contents());
  if (api_web_contents) {
    api_web_contents->Invoke(internal, channel, std::move(arguments),
                             std::move(callback), GetRenderFrameHost());
  }
}

void ElectronBrowserHandlerImpl::OnFirstNonEmptyLayout() {
  api::WebContents* api_web_contents = api::WebContents::From(web_contents());
  if (api_web_contents) {
    api_web_contents->OnFirstNonEmptyLayout(GetRenderFrameHost());
  }
}

void ElectronBrowserHandlerImpl::ReceivePostMessage(
    const std::string& channel,
    blink::TransferableMessage message) {
  api::WebContents* api_web_contents = api::WebContents::From(web_contents());
  if (api_web_contents) {
    api_web_contents->ReceivePostMessage(channel, std::move(message),
                                         GetRenderFrameHost());
  }
}

void ElectronBrowserHandlerImpl::MessageSync(bool internal,
                                             const std::string& channel,
                                             blink::CloneableMessage arguments,
                                             MessageSyncCallback callback) {
  api::WebContents* api_web_contents = api::WebContents::From(web_contents());
  if (api_web_contents) {
    api_web_contents->MessageSync(internal, channel, std::move(arguments),
                                  std::move(callback), GetRenderFrameHost());
  }
}

void ElectronBrowserHandlerImpl::MessageTo(int32_t web_contents_id,
                                           const std::string& channel,
                                           blink::CloneableMessage arguments) {
  api::WebContents* api_web_contents = api::WebContents::From(web_contents());
  if (api_web_contents) {
    api_web_contents->MessageTo(web_contents_id, channel, std::move(arguments));
  }
}

void ElectronBrowserHandlerImpl::MessageHost(
    const std::string& channel,
    blink::CloneableMessage arguments) {
  api::WebContents* api_web_contents = api::WebContents::From(web_contents());
  if (api_web_contents) {
    api_web_contents->MessageHost(channel, std::move(arguments),
                                  GetRenderFrameHost());
  }
}

void ElectronBrowserHandlerImpl::UpdateDraggableRegions(
    std::vector<mojom::DraggableRegionPtr> regions) {
  api::WebContents* api_web_contents = api::WebContents::From(web_contents());
  if (api_web_contents) {
    api_web_contents->UpdateDraggableRegions(std::move(regions));
  }
}

void ElectronBrowserHandlerImpl::SetTemporaryZoomLevel(double level) {
  api::WebContents* api_web_contents = api::WebContents::From(web_contents());
  if (api_web_contents) {
    api_web_contents->SetTemporaryZoomLevel(level);
  }
}

void ElectronBrowserHandlerImpl::DoGetZoomLevel(
    DoGetZoomLevelCallback callback) {
  api::WebContents* api_web_contents = api::WebContents::From(web_contents());
  if (api_web_contents) {
    api_web_contents->DoGetZoomLevel(std::move(callback));
  }
}

content::RenderFrameHost* ElectronBrowserHandlerImpl::GetRenderFrameHost() {
  return content::RenderFrameHost::FromID(render_process_id_, render_frame_id_);
}

// static
void ElectronBrowserHandlerImpl::Create(
    content::RenderFrameHost* frame_host,
    mojo::PendingReceiver<mojom::ElectronBrowser> receiver) {
  new ElectronBrowserHandlerImpl(frame_host, std::move(receiver));
}
}  // namespace electron
