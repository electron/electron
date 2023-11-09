// Copyright (c) 2022 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/electron_api_ipc_handler_impl.h"

#include <utility>

#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "mojo/public/cpp/bindings/self_owned_receiver.h"

namespace electron {
ElectronApiIPCHandlerImpl::ElectronApiIPCHandlerImpl(
    content::RenderFrameHost* frame_host,
    mojo::PendingAssociatedReceiver<mojom::ElectronApiIPC> receiver)
    : render_frame_host_id_(frame_host->GetGlobalId()) {
  content::WebContents* web_contents =
      content::WebContents::FromRenderFrameHost(frame_host);
  DCHECK(web_contents);
  content::WebContentsObserver::Observe(web_contents);

  receiver_.Bind(std::move(receiver));
  receiver_.set_disconnect_handler(base::BindOnce(
      &ElectronApiIPCHandlerImpl::OnConnectionError, GetWeakPtr()));
}

ElectronApiIPCHandlerImpl::~ElectronApiIPCHandlerImpl() = default;

void ElectronApiIPCHandlerImpl::WebContentsDestroyed() {
  delete this;
}

void ElectronApiIPCHandlerImpl::OnConnectionError() {
  delete this;
}

void ElectronApiIPCHandlerImpl::Message(bool internal,
                                        const std::string& channel,
                                        blink::CloneableMessage arguments) {
  api::WebContents* api_web_contents = api::WebContents::From(web_contents());
  if (api_web_contents) {
    api_web_contents->Message(internal, channel, std::move(arguments),
                              GetRenderFrameHost());
  }
}
void ElectronApiIPCHandlerImpl::Invoke(bool internal,
                                       const std::string& channel,
                                       blink::CloneableMessage arguments,
                                       InvokeCallback callback) {
  api::WebContents* api_web_contents = api::WebContents::From(web_contents());
  if (api_web_contents) {
    api_web_contents->Invoke(internal, channel, std::move(arguments),
                             std::move(callback), GetRenderFrameHost());
  }
}

void ElectronApiIPCHandlerImpl::ReceivePostMessage(
    const std::string& channel,
    blink::TransferableMessage message) {
  api::WebContents* api_web_contents = api::WebContents::From(web_contents());
  if (api_web_contents) {
    api_web_contents->ReceivePostMessage(channel, std::move(message),
                                         GetRenderFrameHost());
  }
}

void ElectronApiIPCHandlerImpl::MessageSync(bool internal,
                                            const std::string& channel,
                                            blink::CloneableMessage arguments,
                                            MessageSyncCallback callback) {
  api::WebContents* api_web_contents = api::WebContents::From(web_contents());
  if (api_web_contents) {
    api_web_contents->MessageSync(internal, channel, std::move(arguments),
                                  std::move(callback), GetRenderFrameHost());
  }
}

void ElectronApiIPCHandlerImpl::MessageHost(const std::string& channel,
                                            blink::CloneableMessage arguments) {
  api::WebContents* api_web_contents = api::WebContents::From(web_contents());
  if (api_web_contents) {
    api_web_contents->MessageHost(channel, std::move(arguments),
                                  GetRenderFrameHost());
  }
}

content::RenderFrameHost* ElectronApiIPCHandlerImpl::GetRenderFrameHost() {
  return content::RenderFrameHost::FromID(render_frame_host_id_);
}

// static
void ElectronApiIPCHandlerImpl::Create(
    content::RenderFrameHost* frame_host,
    mojo::PendingAssociatedReceiver<mojom::ElectronApiIPC> receiver) {
  new ElectronApiIPCHandlerImpl(frame_host, std::move(receiver));
}
}  // namespace electron
