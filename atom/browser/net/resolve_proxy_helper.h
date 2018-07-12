// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_NET_RESOLVE_PROXY_HELPER_H_
#define ATOM_BROWSER_NET_RESOLVE_PROXY_HELPER_H_

#include <deque>
#include <string>

#include "base/memory/ref_counted.h"
#include "net/proxy/proxy_service.h"
#include "url/gurl.h"

namespace net {
class URLRequestContextGetter;
}

namespace atom {

class ResolveProxyHelper
    : public base::RefCountedThreadSafe<ResolveProxyHelper> {
 public:
  using ResolveProxyCallback = base::Callback<void(std::string)>;
  explicit ResolveProxyHelper(net::URLRequestContextGetter* getter);

  void ResolveProxy(const GURL& url, const ResolveProxyCallback& callback);

 private:
  friend class base::RefCountedThreadSafe<ResolveProxyHelper>;
  struct PendingRequest {
   public:
    PendingRequest(const GURL& url, const ResolveProxyCallback& callback)
        : url(url), callback(callback), pac_req(nullptr) {}

    GURL url;
    ResolveProxyCallback callback;
    net::ProxyService::PacRequest* pac_req;
  };

  ~ResolveProxyHelper();

  void StartPendingRequest();
  void StartPendingRequestInIO(const GURL& request,
                               net::ProxyService::PacRequest* pac_req);
  void SendProxyResult(const std::string& proxy);
  void OnResolveProxyCompleted(int result);

  net::ProxyInfo proxy_info_;
  std::deque<PendingRequest> pending_requests_;
  scoped_refptr<net::URLRequestContextGetter> context_getter_;
  scoped_refptr<base::SingleThreadTaskRunner> original_thread_;

  DISALLOW_COPY_AND_ASSIGN(ResolveProxyHelper);
};

}  // namespace atom

#endif  // ATOM_BROWSER_NET_RESOLVE_PROXY_HELPER_H_
