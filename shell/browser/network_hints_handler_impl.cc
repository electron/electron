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

NetworkHintsHandlerImpl::NetworkHintsHandlerImpl(int32_t render_process_id)
    : network_hints::SimpleNetworkHintsHandlerImpl(render_process_id),
      render_process_id_(render_process_id) {}

NetworkHintsHandlerImpl::~NetworkHintsHandlerImpl() = default;

void NetworkHintsHandlerImpl::Preconnect(int32_t render_frame_id,
                                         const GURL& url,
                                         bool allow_credentials) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  content::RenderFrameHost* render_frame_host =
      content::RenderFrameHost::FromID(render_process_id_, render_frame_id);
  content::BrowserContext* browser_context =
      render_frame_host->GetProcess()->GetBrowserContext();
  auto* session = electron::api::Session::FromWrappedClass(
      v8::Isolate::GetCurrent(),
      static_cast<electron::AtomBrowserContext*>(browser_context));
  if (session) {
    session->Emit("preconnect", url, allow_credentials);
  }
}

void NetworkHintsHandlerImpl::Create(
    int32_t render_process_id,
    mojo::PendingReceiver<network_hints::mojom::NetworkHintsHandler> receiver) {
  mojo::MakeSelfOwnedReceiver(
      base::WrapUnique(new NetworkHintsHandlerImpl(render_process_id)),
      std::move(receiver));
}
