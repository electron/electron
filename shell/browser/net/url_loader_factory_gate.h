// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_NET_URL_LOADER_FACTORY_GATE_H_
#define ELECTRON_SHELL_BROWSER_NET_URL_LOADER_FACTORY_GATE_H_

#include <string>
#include <vector>

#include "base/containers/flat_set.h"
#include "base/memory/ref_counted.h"
#include "base/synchronization/lock.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "services/network/public/mojom/url_loader_factory.mojom-forward.h"

class GURL;

namespace electron {

// What makes a session's requests take the ProxyingURLLoaderFactory path: any
// webRequest listener (they re-run on redirects, so URL filters cannot be
// applied here), an intercepted scheme, or an --ignore-connections-limit
// domain. Written on the UI thread, read on the IO thread.
class InterceptState : public base::RefCountedThreadSafe<InterceptState> {
 public:
  InterceptState();

  void SetHasWebRequestListeners(bool has_listeners);
  void SetInterceptedSchemes(base::flat_set<std::string> schemes);
  void SetIgnoreConnectionsLimitDomains(std::vector<std::string> domains);

  // True when a request for `url` has to go through ProxyingURLLoaderFactory.
  bool WantsRequest(const GURL& url) const;

 private:
  friend class base::RefCountedThreadSafe<InterceptState>;
  ~InterceptState();

  mutable base::Lock lock_;
  bool has_web_request_listeners_ GUARDED_BY(lock_) = false;
  base::flat_set<std::string> intercepted_schemes_ GUARDED_BY(lock_);
  std::vector<std::string> ignore_connections_limit_domains_ GUARDED_BY(lock_);
};

// Bound on the IO thread between a renderer's factory pipe and the network:
// requests go straight to `target` unless `state` wants them on the UI thread,
// in which case `interceptor` (a ProxyingURLLoaderFactory) gets them.
void CreateURLLoaderFactoryGate(
    scoped_refptr<InterceptState> state,
    mojo::PendingReceiver<network::mojom::URLLoaderFactory> receiver,
    mojo::PendingRemote<network::mojom::URLLoaderFactory> target,
    mojo::PendingRemote<network::mojom::URLLoaderFactory> interceptor);

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_NET_URL_LOADER_FACTORY_GATE_H_
