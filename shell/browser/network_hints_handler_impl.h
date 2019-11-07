// Copyright (c) 2019 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_NETWORK_HINTS_HANDLER_IMPL_H_
#define SHELL_BROWSER_NETWORK_HINTS_HANDLER_IMPL_H_

#include "components/network_hints/browser/simple_network_hints_handler_impl.h"

class NetworkHintsHandlerImpl
    : public network_hints::SimpleNetworkHintsHandlerImpl {
 public:
  ~NetworkHintsHandlerImpl() override;

  static void Create(
      int32_t render_process_id,
      mojo::PendingReceiver<network_hints::mojom::NetworkHintsHandler>
          receiver);

  // network_hints::mojom::NetworkHintsHandler:
  void Preconnect(int32_t render_frame_id,
                  const GURL& url,
                  bool allow_credentials) override;

 private:
  explicit NetworkHintsHandlerImpl(int32_t render_process_id);

  int32_t render_process_id_;
};

#endif  // SHELL_BROWSER_NETWORK_HINTS_HANDLER_IMPL_H_
