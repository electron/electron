// Copyright (c) 2019 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_NETWORK_HINTS_HANDLER_IMPL_H_
#define ELECTRON_SHELL_BROWSER_NETWORK_HINTS_HANDLER_IMPL_H_

#include "components/network_hints/browser/simple_network_hints_handler_impl.h"

namespace content {
class RenderFrameHost;
class BrowserContext;
}  // namespace content

class NetworkHintsHandlerImpl
    : public network_hints::SimpleNetworkHintsHandlerImpl {
 public:
  ~NetworkHintsHandlerImpl() override;

  static void Create(
      content::RenderFrameHost* frame_host,
      mojo::PendingReceiver<network_hints::mojom::NetworkHintsHandler>
          receiver);

  // network_hints::mojom::NetworkHintsHandler:
  void Preconnect(const GURL& url, bool allow_credentials) override;

 private:
  explicit NetworkHintsHandlerImpl(content::RenderFrameHost*);

  content::BrowserContext* browser_context_ = nullptr;
};

#endif  // ELECTRON_SHELL_BROWSER_NETWORK_HINTS_HANDLER_IMPL_H_
