// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_NET_URL_LOADER_HEADER_CLIENT_CHAIN_H_
#define ELECTRON_SHELL_BROWSER_NET_URL_LOADER_HEADER_CLIENT_CHAIN_H_

#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "services/network/public/mojom/network_context.mojom-forward.h"

namespace electron {

// The network service takes one TrustedURLLoaderHeaderClient per factory.
// This binds `receiver` to an object that shares it between two proxying
// URLLoaderFactories on the same factory chain: `outer` belongs to the proxy
// nearer the renderer and `inner` to the one nearer the network. Request
// headers go to `outer` then `inner`, and response headers to `inner` then
// `outer`, the order in which the rest of each request reaches the proxies.
//
// Both proxies have to know a loader by the request ID the network service
// reports, so `inner` must forward the request ID it is given.
//
// The object lives until `receiver` is disconnected.
void ChainURLLoaderHeaderClients(
    mojo::PendingReceiver<network::mojom::TrustedURLLoaderHeaderClient>
        receiver,
    mojo::PendingRemote<network::mojom::TrustedURLLoaderHeaderClient> outer,
    mojo::PendingRemote<network::mojom::TrustedURLLoaderHeaderClient> inner);

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_NET_URL_LOADER_HEADER_CLIENT_CHAIN_H_
