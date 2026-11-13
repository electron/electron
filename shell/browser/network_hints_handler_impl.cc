// Copyright (c) 2019 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/network_hints_handler_impl.h"

#include <utility>

#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"
#include "mojo/public/cpp/bindings/self_owned_receiver.h"
#include "shell/browser/api/electron_api_session.h"
#include "shell/browser/api/electron_api_web_frame_main.h"
#include "shell/browser/javascript_environment.h"
#include "shell/common/gin_converters/frame_converter.h"
#include "shell/common/gin_converters/gurl_converter.h"
#include "v8/include/v8.h"

NetworkHintsHandlerImpl::NetworkHintsHandlerImpl(
    content::RenderFrameHost* frame_host)
    : network_hints::SimpleNetworkHintsHandlerImpl(frame_host->GetGlobalId()),
      render_frame_host_id_(frame_host->GetGlobalId()) {}

NetworkHintsHandlerImpl::~NetworkHintsHandlerImpl() = default;

void NetworkHintsHandlerImpl::Preconnect(const url::SchemeHostPort& url,
                                         bool allow_credentials) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  auto* frame_host = content::RenderFrameHost::FromID(render_frame_host_id_);
  if (!frame_host) {
    return;
  }
  v8::Isolate* isolate = electron::JavascriptEnvironment::GetIsolate();
  v8::HandleScope scope(isolate);
  auto* frame = electron::api::WebFrameMain::From(isolate, frame_host);
  if (!frame)
    return;

  frame->Emit("preconnect", url.GetURL(), allow_credentials);

  // The frame listener may have destroyed the requesting frame.
  frame_host = frame->render_frame_host();
  if (!frame_host)
    return;

  auto* session = electron::api::Session::FromBrowserContext(
      frame_host->GetBrowserContext());
  if (session && session->Get()) {
    session->Get()->Emit("-preconnect", url.GetURL(), allow_credentials,
                         frame_host);
  }
}

void NetworkHintsHandlerImpl::Create(
    content::RenderFrameHost* frame_host,
    mojo::PendingReceiver<network_hints::mojom::NetworkHintsHandler> receiver) {
  mojo::MakeSelfOwnedReceiver(
      base::WrapUnique(new NetworkHintsHandlerImpl(frame_host)),
      std::move(receiver));
}
