// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_IO_THREAD_H_
#define ATOM_BROWSER_IO_THREAD_H_

#include <memory>
#include <set>

#include "atom/browser/net/system_network_context_manager.h"
#include "base/macros.h"
#include "base/synchronization/lock.h"
#include "content/public/browser/browser_thread_delegate.h"
#include "services/network/public/mojom/network_service.mojom.h"

namespace atom {
class URLRequestContextGetter;
}

namespace net {
class URLRequestContext;
}

namespace net_log {
class ChromeNetLog;
}

class IOThread : public content::BrowserThreadDelegate {
 public:
  explicit IOThread(
      net_log::ChromeNetLog* net_log,
      SystemNetworkContextManager* system_network_context_manager);
  ~IOThread() override;

  void RegisterURLRequestContextGetter(atom::URLRequestContextGetter* getter);
  void DeregisterURLRequestContextGetter(atom::URLRequestContextGetter* getter);

 protected:
  // BrowserThreadDelegate Implementation, runs on the IO thread.
  void Init() override;
  void CleanUp() override;

 private:
  // The NetLog is owned by the browser process, to allow logging from other
  // threads during shutdown, but is used most frequently on the IOThread.
  net_log::ChromeNetLog* net_log_;

  // When the network service is disabled, this holds on to a
  // content::NetworkContext class that owns |system_request_context_|.
  std::unique_ptr<network::mojom::NetworkContext> system_network_context_;
  net::URLRequestContext* system_request_context_;

  // These are set on the UI thread, and then consumed during initialization on
  // the IO thread.
  network::mojom::NetworkContextRequest network_context_request_;
  network::mojom::NetworkContextParamsPtr network_context_params_;

  // Initial HTTP auth configuration used when setting up the NetworkService on
  // the IO Thread. Future updates are sent using the NetworkService mojo
  // interface, but initial state needs to be set non-racily.
  network::mojom::HttpAuthStaticParamsPtr http_auth_static_params_;
  network::mojom::HttpAuthDynamicParamsPtr http_auth_dynamic_params_;

  // |lock_| protects access to |request_context_getters_|.
  base::Lock lock_;

  // List of all request contexts that needs to be notified when
  // IO thread is shutting down.
  std::set<atom::URLRequestContextGetter*> request_context_getters_;

  DISALLOW_COPY_AND_ASSIGN(IOThread);
};

#endif  // ATOM_BROWSER_IO_THREAD_H_
