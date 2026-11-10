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
// URLLoaderFactories on the same factory chain. Request headers and response
// headers both go to `first` and then, as `first` left them, to `second`, so
// `second` decides what is sent and what the response says.
//
// Both proxies have to know a loader by the request ID the network service
// reports.
//
// `second_handles_loader` is asked about each loader the network service
// reports, by its request ID, or with no ID for a CORS preflight. Where the
// answer is no, `first` is the loader's only header client and is called by the
// network service directly, so a proxy with nothing to do for a request costs
// it nothing.
//
// `second_loader_failed` stands in for `second`'s
// OnLoaderFailedWithRemoteEndpoint(), so that the address reaches it without
// the delay of a relayed message. A proxy can only use the address before the
// loader's OnComplete(), which the network service sends on another pipe.
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
    mojo::PendingRemote<network::mojom::TrustedURLLoaderHeaderClient> first,
    mojo::PendingRemote<network::mojom::TrustedURLLoaderHeaderClient> second,
    HandlesLoaderCallback second_handles_loader,
    LoaderFailedCallback second_loader_failed);

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_NET_URL_LOADER_HEADER_CLIENT_CHAIN_H_
