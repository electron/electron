// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_NET_RESOLVE_PROXY_HELPER_H_
#define ATOM_BROWSER_NET_RESOLVE_PROXY_HELPER_H_

#include <deque>
#include <string>

#include "base/memory/ref_counted.h"
#include "base/optional.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/network/public/mojom/proxy_lookup_client.mojom.h"
#include "url/gurl.h"

namespace atom {

class AtomBrowserContext;

class ResolveProxyHelper
    : public base::RefCountedThreadSafe<ResolveProxyHelper>,
      network::mojom::ProxyLookupClient {
 public:
  using ResolveProxyCallback = base::Callback<void(std::string)>;

  explicit ResolveProxyHelper(AtomBrowserContext* browser_context);

  void ResolveProxy(const GURL& url, const ResolveProxyCallback& callback);

 protected:
  ~ResolveProxyHelper() override;

 private:
  friend class base::RefCountedThreadSafe<ResolveProxyHelper>;
  // A PendingRequest is a resolve request that is in progress, or queued.
  struct PendingRequest {
   public:
    PendingRequest(const GURL& url, const ResolveProxyCallback& callback);
    PendingRequest(PendingRequest&& pending_request) noexcept;
    ~PendingRequest();

    PendingRequest& operator=(PendingRequest&& pending_request) noexcept;

    GURL url;
    ResolveProxyCallback callback;

   private:
    DISALLOW_COPY_AND_ASSIGN(PendingRequest);
  };

  // Starts the first pending request.
  void StartPendingRequest();

  // network::mojom::ProxyLookupClient implementation.
  void OnProxyLookupComplete(
      int32_t net_error,
      const base::Optional<net::ProxyInfo>& proxy_info) override;

  // Self-reference. Owned as long as there's an outstanding proxy lookup.
  scoped_refptr<ResolveProxyHelper> owned_self_;

  std::deque<PendingRequest> pending_requests_;
  // Binding for the currently in-progress request, if any.
  mojo::Binding<network::mojom::ProxyLookupClient> binding_;

  // Weak Ref
  AtomBrowserContext* browser_context_;

  DISALLOW_COPY_AND_ASSIGN(ResolveProxyHelper);
};

}  // namespace atom

#endif  // ATOM_BROWSER_NET_RESOLVE_PROXY_HELPER_H_
