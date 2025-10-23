// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_NET_RESOLVE_PROXY_HELPER_H_
#define ELECTRON_SHELL_BROWSER_NET_RESOLVE_PROXY_HELPER_H_

#include <optional>
#include <string>

#include "base/containers/circular_deque.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/ref_counted.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "services/network/public/mojom/proxy_lookup_client.mojom.h"
#include "url/gurl.h"

namespace electron {

class ElectronBrowserContext;

class ResolveProxyHelper
    : public base::RefCountedThreadSafe<ResolveProxyHelper>,
      network::mojom::ProxyLookupClient {
 public:
  using ResolveProxyCallback = base::OnceCallback<void(std::string)>;

  explicit ResolveProxyHelper(ElectronBrowserContext* browser_context);

  void ResolveProxy(const GURL& url, ResolveProxyCallback callback);

  // disable copy
  ResolveProxyHelper(const ResolveProxyHelper&) = delete;
  ResolveProxyHelper& operator=(const ResolveProxyHelper&) = delete;

 protected:
  ~ResolveProxyHelper() override;

 private:
  friend class base::RefCountedThreadSafe<ResolveProxyHelper>;
  // A PendingRequest is a resolve request that is in progress, or queued.
  struct PendingRequest {
   public:
    PendingRequest(const GURL& url, ResolveProxyCallback callback);
    PendingRequest(PendingRequest&& pending_request) noexcept;
    ~PendingRequest();

    // disable copy
    PendingRequest(const PendingRequest&) = delete;
    PendingRequest& operator=(const PendingRequest&) = delete;

    PendingRequest& operator=(PendingRequest&& pending_request) noexcept;

    GURL url;
    ResolveProxyCallback callback;
  };

  // Starts the first pending request.
  void StartPendingRequest();

  // network::mojom::ProxyLookupClient implementation.
  void OnProxyLookupComplete(
      int32_t net_error,
      const std::optional<net::ProxyInfo>& proxy_info) override;

  // Self-reference. Owned as long as there's an outstanding proxy lookup.
  scoped_refptr<ResolveProxyHelper> owned_self_;

  base::circular_deque<PendingRequest> pending_requests_;
  // Receiver for the currently in-progress request, if any.
  mojo::Receiver<network::mojom::ProxyLookupClient> receiver_{this};

  // Weak Ref
  raw_ptr<ElectronBrowserContext> browser_context_;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_NET_RESOLVE_PROXY_HELPER_H_
