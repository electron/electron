// Copyright (c) 2019 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/network_hints_handler_impl.h"

#include <utility>

#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "mojo/public/cpp/bindings/self_owned_receiver.h"
#include "shell/browser/api/atom_api_session.h"
#include "shell/browser/atom_browser_context.h"
#include "shell/common/gin_converters/gurl_converter.h"
#include "v8/include/v8.h"

NetworkHintsHandlerImpl::NetworkHintsHandlerImpl(
    content::RenderFrameHost* frame_host)
    : network_hints::SimpleNetworkHintsHandlerImpl(
          frame_host->GetProcess()->GetID()),
      render_frame_host_(frame_host) {}

NetworkHintsHandlerImpl::~NetworkHintsHandlerImpl() = default;

void NetworkHintsHandlerImpl::Preconnect(const GURL& url,
                                         bool allow_credentials) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  content::BrowserContext* browser_context =
      render_frame_host_->GetProcess()->GetBrowserContext();
  auto* session = electron::api::Session::FromWrappedClass(
      v8::Isolate::GetCurrent(),
      static_cast<electron::AtomBrowserContext*>(browser_context));
  if (session) {
    session->Emit("preconnect", url, allow_credentials);
  }
}

void NetworkHintsHandlerImpl::Create(
    content::RenderFrameHost* frame_host,
    mojo::PendingReceiver<network_hints::mojom::NetworkHintsHandler> receiver) {
  mojo::MakeSelfOwnedReceiver(
      base::WrapUnique(new NetworkHintsHandlerImpl(frame_host)),
      std::move(receiver));
}
