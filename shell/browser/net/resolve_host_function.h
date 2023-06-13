// Copyright (c) 2023 Signal Messenger, LLC
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_NET_RESOLVE_HOST_FUNCTION_H_
#define ELECTRON_SHELL_BROWSER_NET_RESOLVE_HOST_FUNCTION_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "base/memory/ref_counted.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "net/base/address_list.h"
#include "net/dns/public/host_resolver_results.h"
#include "services/network/public/cpp/resolve_host_client_base.h"
#include "services/network/public/mojom/host_resolver.mojom.h"
#include "services/network/public/mojom/network_context.mojom.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace electron {

class ElectronBrowserContext;

class ResolveHostFunction
    : public base::RefCountedThreadSafe<ResolveHostFunction>,
      network::ResolveHostClientBase {
 public:
  using ResolveHostCallback = base::OnceCallback<void(
      int64_t,
      const absl::optional<net::AddressList>& resolved_addresses)>;

  explicit ResolveHostFunction(ElectronBrowserContext* browser_context,
                               std::string host,
                               network::mojom::ResolveHostParametersPtr params,
                               ResolveHostCallback callback);

  void Run();

  // disable copy
  ResolveHostFunction(const ResolveHostFunction&) = delete;
  ResolveHostFunction& operator=(const ResolveHostFunction&) = delete;

 protected:
  ~ResolveHostFunction() override;

 private:
  friend class base::RefCountedThreadSafe<ResolveHostFunction>;

  // network::mojom::ResolveHostClient implementation
  void OnComplete(int result,
                  const net::ResolveErrorInfo& resolve_error_info,
                  const absl::optional<net::AddressList>& resolved_addresses,
                  const absl::optional<net::HostResolverEndpointResults>&
                      endpoint_results_with_metadata) override;

  // Receiver for the currently in-progress request, if any.
  mojo::Receiver<network::mojom::ResolveHostClient> receiver_{this};

  // Weak Ref
  raw_ptr<ElectronBrowserContext> browser_context_;
  std::string host_;
  network::mojom::ResolveHostParametersPtr params_;
  ResolveHostCallback callback_;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_NET_RESOLVE_HOST_FUNCTION_H_
