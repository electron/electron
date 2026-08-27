// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_NET_URL_LOADER_FACTORY_GATE_H_
#define ELECTRON_SHELL_BROWSER_NET_URL_LOADER_FACTORY_GATE_H_

#include <stdint.h>

#include <string>
#include <vector>

#include "base/containers/flat_set.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/synchronization/lock.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "services/network/public/mojom/url_loader.mojom-forward.h"
#include "services/network/public/mojom/url_loader_factory.mojom-forward.h"

class GURL;

namespace extensions {
enum class WebRequestResourceType : uint8_t;
}

namespace network {
struct ResourceRequest;
}

namespace net {
struct MutableNetworkTrafficAnnotationTag;
}

namespace electron {

class ElectronBrowserContext;

// The webRequest resource type of a renderer request, as WebRequestInfo would
// compute it. Unlike the URL it cannot change across redirects.
extensions::WebRequestResourceType ResourceTypeOf(
    const network::ResourceRequest& request);

constexpr uint32_t kAllResourceTypes = ~0u;

// What makes a session's requests leave the direct renderer -> network path:
// a webRequest listener that can block or modify them (routed through
// ProxyingURLLoaderFactory on the UI thread), one that only observes them
// (reported to the UI thread after the fact), an intercepted scheme, or an
// --ignore-connections-limit domain. Written on the UI thread, read on IO.
class InterceptState : public base::RefCountedThreadSafe<InterceptState> {
 public:
  enum class Route { kDirect, kObserve, kProxy };

  InterceptState();

  // Bit N set = some listener of that kind matches WebRequestResourceType N.
  void SetListenerTypes(uint32_t blocking_types, uint32_t observer_types);
  void SetInterceptedSchemes(base::flat_set<std::string> schemes);
  void SetIgnoreConnectionsLimitDomains(std::vector<std::string> domains);

  Route RouteFor(const network::ResourceRequest& request) const;

 private:
  friend class base::RefCountedThreadSafe<InterceptState>;
  ~InterceptState();

  bool WantsURL(const GURL& url) const EXCLUSIVE_LOCKS_REQUIRED(lock_);

  mutable base::Lock lock_;
  uint32_t blocking_types_ GUARDED_BY(lock_) = 0;
  uint32_t observer_types_ GUARDED_BY(lock_) = 0;
  base::flat_set<std::string> intercepted_schemes_ GUARDED_BY(lock_);
  std::vector<std::string> ignore_connections_limit_domains_ GUARDED_BY(lock_);
};

// Starts a request on the IO thread with both URLLoader endpoints proxied so
// observer only webRequest events can be reported without blocking the load.
void CreateObservedLoaderAndStartOnIO(
    base::WeakPtr<ElectronBrowserContext> browser_context,
    int render_process_id,
    int frame_routing_id,
    mojo::PendingReceiver<network::mojom::URLLoader> loader,
    int32_t request_id,
    uint32_t options,
    const network::ResourceRequest& request,
    mojo::PendingRemote<network::mojom::URLLoaderClient> client,
    const net::MutableNetworkTrafficAnnotationTag& traffic_annotation,
    mojo::PendingRemote<network::mojom::URLLoaderFactory> target);

// Bound on the IO thread between a renderer's factory pipe and the network:
// requests go straight to `target` unless `state` wants them on the UI thread,
// in which case `interceptor` (a ProxyingURLLoaderFactory) gets them. Lives as
// long as its receivers and both remotes do.
void CreateURLLoaderFactoryGate(
    ElectronBrowserContext* browser_context,
    int render_process_id,
    int frame_routing_id,
    mojo::PendingReceiver<network::mojom::URLLoaderFactory> receiver,
    mojo::PendingRemote<network::mojom::URLLoaderFactory> target,
    mojo::PendingRemote<network::mojom::URLLoaderFactory> interceptor);

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_NET_URL_LOADER_FACTORY_GATE_H_
