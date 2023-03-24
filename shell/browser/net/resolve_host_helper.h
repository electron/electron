// Copyright (c) 2023 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_NET_RESOLVE_HOST_HELPER_H_
#define ELECTRON_SHELL_BROWSER_NET_RESOLVE_HOST_HELPER_H_

#include <deque>
#include <string>
#include <vector>

#include "base/memory/ref_counted.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "net/base/address_list.h"
#include "net/dns/public/host_resolver_results.h"
#include "services/network/public/cpp/resolve_host_client_base.h"
#include "services/network/public/mojom/network_context.mojom.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace electron {

class ElectronBrowserContext;

class ResolveHostHelper : public base::RefCountedThreadSafe<ResolveHostHelper>,
                          network::ResolveHostClientBase {
 public:
  using ResolveHostCallback =
      base::OnceCallback<void(int64_t, std::vector<std::string>)>;

  explicit ResolveHostHelper(ElectronBrowserContext* browser_context);

  void ResolveHost(std::string host, ResolveHostCallback callback);

  // disable copy
  ResolveHostHelper(const ResolveHostHelper&) = delete;
  ResolveHostHelper& operator=(const ResolveHostHelper&) = delete;

 protected:
  ~ResolveHostHelper() override;

 private:
  friend class base::RefCountedThreadSafe<ResolveHostHelper>;
  // A PendingRequest is a resolve request that is in progress, or queued.
  struct PendingRequest {
   public:
    PendingRequest(std::string host, ResolveHostCallback callback);
    PendingRequest(PendingRequest&& pending_request) noexcept;
    ~PendingRequest();

    // disable copy
    PendingRequest(const PendingRequest&) = delete;
    PendingRequest& operator=(const PendingRequest&) = delete;

    PendingRequest& operator=(PendingRequest&& pending_request) noexcept;

    std::string host;
    ResolveHostCallback callback;
  };

  // Starts the first pending request.
  void StartPendingRequest();

  // network::mojom::ResolveHostClient implementation
  void OnComplete(int result,
                  const net::ResolveErrorInfo& resolve_error_info,
                  const absl::optional<net::AddressList>& resolved_addresses,
                  const absl::optional<net::HostResolverEndpointResults>&
                      endpoint_results_with_metadata) override;

  // Self-reference. Owned as long as there's an outstanding host lookup.
  scoped_refptr<ResolveHostHelper> owned_self_;

  std::deque<PendingRequest> pending_requests_;
  // Receiver for the currently in-progress request, if any.
  mojo::Receiver<network::mojom::ResolveHostClient> receiver_{this};

  // Weak Ref
  ElectronBrowserContext* browser_context_;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_NET_RESOLVE_HOST_HELPER_H_
