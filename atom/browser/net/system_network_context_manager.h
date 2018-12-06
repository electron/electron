// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_NET_SYSTEM_NETWORK_CONTEXT_MANAGER_H_
#define ATOM_BROWSER_NET_SYSTEM_NETWORK_CONTEXT_MANAGER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/optional.h"
#include "chrome/browser/net/proxy_config_monitor.h"
#include "services/network/public/mojom/network_context.mojom.h"
#include "services/network/public/mojom/network_service.mojom.h"

namespace network {
namespace mojom {
class URLLoaderFactory;
}
class SharedURLLoaderFactory;
}  // namespace network

namespace net_log {
class NetExportFileWriter;
}

// Responsible for creating and managing access to the system NetworkContext.
// Lives on the UI thread. The NetworkContext this owns is intended for requests
// not associated with a session. It stores no data on disk, and has no HTTP
// cache, but it does have ephemeral cookie and channel ID stores.
//
// This class is also responsible for configuring global NetworkService state.
//
// The "system" NetworkContext will either share a URLRequestContext with
// IOThread's SystemURLRequestContext and be part of IOThread's NetworkService
// (If the network service is disabled) or be an independent NetworkContext
// using the actual network service.
class SystemNetworkContextManager {
 public:
  SystemNetworkContextManager();
  ~SystemNetworkContextManager();

  // Returns default set of parameters for configuring the network service.
  static network::mojom::NetworkContextParamsPtr
  CreateDefaultNetworkContextParams();

  // Initializes |network_context_params| as needed to set up a system
  // NetworkContext. If the network service is disabled,
  // |network_context_request| will be for the NetworkContext used by the
  // SystemNetworkContextManager. Otherwise, this method can still be used to
  // help set up the IOThread's in-process URLRequestContext.
  //
  // Must be called before the system NetworkContext is first used.
  void SetUp(
      network::mojom::NetworkContextRequest* network_context_request,
      network::mojom::NetworkContextParamsPtr* network_context_params,
      network::mojom::HttpAuthStaticParamsPtr* http_auth_static_params,
      network::mojom::HttpAuthDynamicParamsPtr* http_auth_dynamic_params);

  // Returns the System NetworkContext. May only be called after SetUp(). Does
  // any initialization of the NetworkService that may be needed when first
  // called.
  network::mojom::NetworkContext* GetContext();

  // Returns a URLLoaderFactory owned by the SystemNetworkContextManager that is
  // backed by the SystemNetworkContext. Allows sharing of the URLLoaderFactory.
  // Prefer this to creating a new one.  Call Clone() on the value returned by
  // this method to get a URLLoaderFactory that can be used on other threads.
  network::mojom::URLLoaderFactory* GetURLLoaderFactory();

  // Returns a SharedURLLoaderFactory owned by the SystemNetworkContextManager
  // that is backed by the SystemNetworkContext.
  scoped_refptr<network::SharedURLLoaderFactory> GetSharedURLLoaderFactory();

  // Returns a shared global NetExportFileWriter instance.
  net_log::NetExportFileWriter* GetNetExportFileWriter();

  // Called when content creates a NetworkService. Creates the
  // SystemNetworkContext, if the network service is enabled.
  void OnNetworkServiceCreated(network::mojom::NetworkService* network_service);

 private:
  class URLLoaderFactoryForSystem;

  // Creates parameters for the NetworkContext. May only be called once, since
  // it initializes some class members.
  network::mojom::NetworkContextParamsPtr CreateNetworkContextParams();

  ProxyConfigMonitor proxy_config_monitor_;

  // NetworkContext using the network service, if the network service is
  // enabled. nullptr, otherwise.
  network::mojom::NetworkContextPtr network_service_network_context_;

  // This is a NetworkContext that wraps the IOThread's SystemURLRequestContext.
  // Always initialized in SetUp, but it's only returned by Context() when the
  // network service is disabled.
  network::mojom::NetworkContextPtr io_thread_network_context_;

  // URLLoaderFactory backed by the NetworkContext returned by GetContext(), so
  // consumers don't all need to create their own factory.
  scoped_refptr<URLLoaderFactoryForSystem> shared_url_loader_factory_;
  network::mojom::URLLoaderFactoryPtr url_loader_factory_;

  // Initialized on first access.
  std::unique_ptr<net_log::NetExportFileWriter> net_export_file_writer_;

  DISALLOW_COPY_AND_ASSIGN(SystemNetworkContextManager);
};

#endif  // ATOM_BROWSER_NET_SYSTEM_NETWORK_CONTEXT_MANAGER_H_
