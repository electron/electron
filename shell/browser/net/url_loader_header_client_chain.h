// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_NET_URL_LOADER_HEADER_CLIENT_CHAIN_H_
#define ELECTRON_SHELL_BROWSER_NET_URL_LOADER_HEADER_CLIENT_CHAIN_H_

#include <optional>

#include "base/functional/callback_forward.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "services/network/public/mojom/network_context.mojom-forward.h"

namespace net {
class IPEndPoint;
}  // namespace net

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
// `inner_handles_loader` is asked about each loader the network service
// reports, by its request ID, or with no ID for a CORS preflight. Where the
// answer is no, `outer` is the loader's only header client and is called by the
// network service directly, so a proxy with nothing to do for a request costs
// it nothing.
//
// `inner_loader_failed` stands in for `inner`'s
// OnLoaderFailedWithRemoteEndpoint(). The address has to reach a proxy before
// the loader's OnComplete(), which the network service sends on another pipe,
// and a message relayed over `inner` would be a step behind it. `outer` gets
// the relayed message, which is in order whenever its OnComplete() comes
// through `inner`.
//
// The object lives until `receiver` is disconnected.
using HandlesLoaderCallback =
    base::RepeatingCallback<bool(std::optional<int32_t> request_id)>;
using LoaderFailedCallback =
    base::RepeatingCallback<void(int32_t request_id,
                                 const net::IPEndPoint& remote_endpoint)>;
void ChainURLLoaderHeaderClients(
    mojo::PendingReceiver<network::mojom::TrustedURLLoaderHeaderClient>
        receiver,
    mojo::PendingRemote<network::mojom::TrustedURLLoaderHeaderClient> outer,
    mojo::PendingRemote<network::mojom::TrustedURLLoaderHeaderClient> inner,
    HandlesLoaderCallback inner_handles_loader,
    LoaderFailedCallback inner_loader_failed);

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_NET_URL_LOADER_HEADER_CLIENT_CHAIN_H_
