// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_NET_RESOLVE_PROXY_HELPER_H_
#define ATOM_BROWSER_NET_RESOLVE_PROXY_HELPER_H_

#include <deque>
#include <string>

#include "base/memory/ref_counted.h"
#include "net/proxy_resolution/proxy_service.h"
#include "url/gurl.h"

namespace net {
class URLRequestContextGetter;
}

namespace atom {

class AtomBrowserContext;

class ResolveProxyHelper
    : public base::RefCountedThreadSafe<ResolveProxyHelper> {
 public:
  using ResolveProxyCallback = base::Callback<void(std::string)>;

  explicit ResolveProxyHelper(AtomBrowserContext* browser_context);

  void ResolveProxy(const GURL& url, const ResolveProxyCallback& callback);

 private:
  friend class base::RefCountedThreadSafe<ResolveProxyHelper>;
  // A PendingRequest is a resolve request that is in progress, or queued.
  struct PendingRequest {
   public:
    PendingRequest(const GURL& url, const ResolveProxyCallback& callback);

    GURL url;
    ResolveProxyCallback callback;
  };

  ~ResolveProxyHelper();

  // Starts the first pending request.
  void StartPendingRequest();
  void StartPendingRequestInIO(const GURL& url);
  void OnProxyResolveComplete(int result);
  void SendProxyResult(const std::string& proxy);

  net::ProxyInfo proxy_info_;
  std::deque<PendingRequest> pending_requests_;
  scoped_refptr<net::URLRequestContextGetter> context_getter_;
  scoped_refptr<base::SingleThreadTaskRunner> original_thread_;

  DISALLOW_COPY_AND_ASSIGN(ResolveProxyHelper);
};

}  // namespace atom

#endif  // ATOM_BROWSER_NET_RESOLVE_PROXY_HELPER_H_
