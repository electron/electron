// Copyright (c) 2023 Signal Messenger, LLC
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/net/resolve_host_function.h"

#include <utility>

#include "base/functional/bind.h"
#include "base/values.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/storage_partition.h"
#include "net/base/host_port_pair.h"
#include "net/base/net_errors.h"
#include "net/base/network_isolation_key.h"
#include "net/dns/public/resolve_error_info.h"
#include "shell/browser/electron_browser_context.h"
#include "url/origin.h"

using content::BrowserThread;

namespace electron {

ResolveHostFunction::ResolveHostFunction(
    ElectronBrowserContext* browser_context,
    std::string host,
    network::mojom::ResolveHostParametersPtr params,
    ResolveHostCallback callback)
    : browser_context_(browser_context),
      host_(std::move(host)),
      params_(std::move(params)),
      callback_(std::move(callback)) {}

ResolveHostFunction::~ResolveHostFunction() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(!receiver_.is_bound());
}

void ResolveHostFunction::Run() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(!receiver_.is_bound());

  // Start the request.
  net::HostPortPair host_port_pair(host_, 0);
  mojo::PendingRemote<network::mojom::ResolveHostClient> resolve_host_client =
      receiver_.BindNewPipeAndPassRemote();
  receiver_.set_disconnect_handler(base::BindOnce(
      &ResolveHostFunction::OnComplete, this, net::ERR_NAME_NOT_RESOLVED,
      net::ResolveErrorInfo(net::ERR_FAILED),
      /*resolved_addresses=*/absl::nullopt,
      /*endpoint_results_with_metadata=*/absl::nullopt));
  browser_context_->GetDefaultStoragePartition()
      ->GetNetworkContext()
      ->ResolveHost(network::mojom::HostResolverHost::NewHostPortPair(
                        std::move(host_port_pair)),
                    net::NetworkAnonymizationKey(), std::move(params_),
                    std::move(resolve_host_client));
}

void ResolveHostFunction::OnComplete(
    int result,
    const net::ResolveErrorInfo& resolve_error_info,
    const absl::optional<net::AddressList>& resolved_addresses,
    const absl::optional<net::HostResolverEndpointResults>&
        endpoint_results_with_metadata) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  // Ensure that we outlive the `receiver_.reset()` call.
  scoped_refptr<ResolveHostFunction> self(this);

  receiver_.reset();

  std::move(callback_).Run(resolve_error_info.error, resolved_addresses);
}

}  // namespace electron
