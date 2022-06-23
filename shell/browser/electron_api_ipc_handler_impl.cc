// Copyright (c) 2022 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/electron_api_ipc_handler_impl.h"

#include <utility>

#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "mojo/public/cpp/bindings/self_owned_receiver.h"

namespace electron {
// static
void ElectronApiIPCHandlerImpl::BindElectronApiIPC(
    mojo::PendingAssociatedReceiver<mojom::ElectronApiIPC> receiver,
    content::RenderFrameHost* rfh) {
  auto* web_contents = content::WebContents::FromRenderFrameHost(rfh);
  DCHECK(web_contents);
  CreateForWebContents(web_contents);
  auto* handler = FromWebContents(web_contents);
  handler->receivers_.Bind(rfh, std::move(receiver));
}

ElectronApiIPCHandlerImpl::ElectronApiIPCHandlerImpl(
    content::WebContents* web_contents)
    : content::WebContentsUserData<ElectronApiIPCHandlerImpl>(*web_contents),
      receivers_(web_contents, this) {}

ElectronApiIPCHandlerImpl::~ElectronApiIPCHandlerImpl() = default;

void ElectronApiIPCHandlerImpl::Message(bool internal,
                                        const std::string& channel,
                                        blink::CloneableMessage arguments) {
  api::WebContents* api_web_contents =
      api::WebContents::From(&GetWebContents());
  if (api_web_contents) {
    api_web_contents->Message(internal, channel, std::move(arguments),
                              receivers_.GetCurrentTargetFrame());
  }
}
void ElectronApiIPCHandlerImpl::Invoke(bool internal,
                                       const std::string& channel,
                                       blink::CloneableMessage arguments,
                                       InvokeCallback callback) {
  api::WebContents* api_web_contents =
      api::WebContents::From(&GetWebContents());
  if (api_web_contents) {
    api_web_contents->Invoke(internal, channel, std::move(arguments),
                             std::move(callback),
                             receivers_.GetCurrentTargetFrame());
  }
}

void ElectronApiIPCHandlerImpl::ReceivePostMessage(
    const std::string& channel,
    blink::TransferableMessage message) {
  api::WebContents* api_web_contents =
      api::WebContents::From(&GetWebContents());
  if (api_web_contents) {
    api_web_contents->ReceivePostMessage(channel, std::move(message),
                                         receivers_.GetCurrentTargetFrame());
  }
}

void ElectronApiIPCHandlerImpl::MessageSync(bool internal,
                                            const std::string& channel,
                                            blink::CloneableMessage arguments,
                                            MessageSyncCallback callback) {
  api::WebContents* api_web_contents =
      api::WebContents::From(&GetWebContents());
  if (api_web_contents) {
    api_web_contents->MessageSync(internal, channel, std::move(arguments),
                                  std::move(callback),
                                  receivers_.GetCurrentTargetFrame());
  }
}

void ElectronApiIPCHandlerImpl::MessageTo(int32_t web_contents_id,
                                          const std::string& channel,
                                          blink::CloneableMessage arguments) {
  api::WebContents* api_web_contents =
      api::WebContents::From(&GetWebContents());
  if (api_web_contents) {
    api_web_contents->MessageTo(web_contents_id, channel, std::move(arguments));
  }
}

void ElectronApiIPCHandlerImpl::MessageHost(const std::string& channel,
                                            blink::CloneableMessage arguments) {
  api::WebContents* api_web_contents =
      api::WebContents::From(&GetWebContents());
  if (api_web_contents) {
    api_web_contents->MessageHost(channel, std::move(arguments),
                                  receivers_.GetCurrentTargetFrame());
  }
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(ElectronApiIPCHandlerImpl);
}  // namespace electron
