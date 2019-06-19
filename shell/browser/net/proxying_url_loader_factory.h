// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_NET_PROXYING_URL_LOADER_FACTORY_H_
#define SHELL_BROWSER_NET_PROXYING_URL_LOADER_FACTORY_H_

#include "shell/browser/net/atom_url_loader_factory.h"

namespace electron {

class ProxyingURLLoaderFactory : public network::mojom::URLLoaderFactory {
 public:
  ProxyingURLLoaderFactory(
      const HandlersMap& handlers,
      network::mojom::URLLoaderFactoryRequest loader_request,
      network::mojom::URLLoaderFactoryPtrInfo target_factory_info);
  ~ProxyingURLLoaderFactory() override;

  // network::mojom::URLLoaderFactory:
  void CreateLoaderAndStart(network::mojom::URLLoaderRequest loader,
                            int32_t routing_id,
                            int32_t request_id,
                            uint32_t options,
                            const network::ResourceRequest& request,
                            network::mojom::URLLoaderClientPtr client,
                            const net::MutableNetworkTrafficAnnotationTag&
                                traffic_annotation) override;
  void Clone(network::mojom::URLLoaderFactoryRequest request) override;

 private:
  void OnTargetFactoryError();
  void OnProxyBindingError();

  // This is passed from api::ProtocolNS.
  //
  // The ProtocolNS instance lives through the lifetime of BrowserContenxt,
  // which is guarenteed to cover the lifetime of URLLoaderFactory, so the
  // reference is guarenteed to be valid.
  //
  // In this way we can avoid using code from api namespace in this file.
  const HandlersMap& handlers_;

  mojo::BindingSet<network::mojom::URLLoaderFactory> proxy_bindings_;
  network::mojom::URLLoaderFactoryPtr target_factory_;

  DISALLOW_COPY_AND_ASSIGN(ProxyingURLLoaderFactory);
};

}  // namespace electron

#endif  // SHELL_BROWSER_NET_PROXYING_URL_LOADER_FACTORY_H_
