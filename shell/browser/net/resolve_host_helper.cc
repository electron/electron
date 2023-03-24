// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/net/resolve_host_helper.h"

#include <utility>
#include <vector>

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

ResolveHostHelper::ResolveHostHelper(ElectronBrowserContext* browser_context)
    : browser_context_(browser_context) {}

ResolveHostHelper::~ResolveHostHelper() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(!owned_self_);
  DCHECK(!receiver_.is_bound());
  // Clear all pending requests if the HostService is still alive.
  pending_requests_.clear();
}

void ResolveHostHelper::ResolveHost(std::string host,
                                    ResolveHostCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  // Enqueue the pending request.
  pending_requests_.emplace_back(host, std::move(callback));

  // If nothing is in progress, start.
  if (!receiver_.is_bound()) {
    DCHECK_EQ(1u, pending_requests_.size());
    StartPendingRequest();
  }
}

void ResolveHostHelper::StartPendingRequest() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(!receiver_.is_bound());
  DCHECK(!pending_requests_.empty());

  // Start the request.
  net::HostPortPair host_port_pair(pending_requests_.front().host, 0);
  mojo::PendingRemote<network::mojom::ResolveHostClient> resolve_host_client =
      receiver_.BindNewPipeAndPassRemote();
  receiver_.set_disconnect_handler(base::BindOnce(
      &ResolveHostHelper::OnComplete, base::Unretained(this),
      net::ERR_NAME_NOT_RESOLVED, net::ResolveErrorInfo(net::ERR_FAILED),
      /*resolved_addresses=*/absl::nullopt,
      /*endpoint_results_with_metadata=*/absl::nullopt));
  browser_context_->GetDefaultStoragePartition()
      ->GetNetworkContext()
      ->ResolveHost(network::mojom::HostResolverHost::NewHostPortPair(
                        std::move(host_port_pair)),
                    net::NetworkAnonymizationKey(), nullptr,
                    std::move(resolve_host_client));
}

void ResolveHostHelper::OnComplete(
    int result,
    const net::ResolveErrorInfo& resolve_error_info,
    const absl::optional<net::AddressList>& resolved_addresses,
    const absl::optional<net::HostResolverEndpointResults>&
        endpoint_results_with_metadata) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(!pending_requests_.empty());

  receiver_.reset();

  // Clear the current (completed) request.
  PendingRequest completed_request = std::move(pending_requests_.front());
  pending_requests_.pop_front();

  std::vector<std::string> addrs;
  if (result == net::OK) {
    DCHECK(resolved_addresses.has_value() && !resolved_addresses->empty());
    addrs.reserve(resolved_addresses->size());

    for (const auto& endpoint : resolved_addresses->endpoints()) {
      addrs.push_back(endpoint.ToStringWithoutPort());
    }
  }

  if (!completed_request.callback.is_null())
    std::move(completed_request.callback).Run(resolve_error_info.error, addrs);

  // Start the next request.
  if (!pending_requests_.empty())
    StartPendingRequest();
}

ResolveHostHelper::PendingRequest::PendingRequest(std::string host,
                                                  ResolveHostCallback callback)
    : host(host), callback(std::move(callback)) {}

ResolveHostHelper::PendingRequest::PendingRequest(
    ResolveHostHelper::PendingRequest&& pending_request) noexcept = default;

ResolveHostHelper::PendingRequest::~PendingRequest() noexcept = default;

ResolveHostHelper::PendingRequest& ResolveHostHelper::PendingRequest::operator=(
    ResolveHostHelper::PendingRequest&& pending_request) noexcept = default;

}  // namespace electron
