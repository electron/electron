// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_NET_PROXYING_URL_LOADER_FACTORY_H_
#define ATOM_BROWSER_NET_PROXYING_URL_LOADER_FACTORY_H_

#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "services/network/public/mojom/url_loader.mojom.h"
#include "services/network/public/mojom/url_loader_factory.mojom.h"

namespace atom {

class ProxyingURLLoaderFactory : public network::mojom::URLLoaderFactory {
 public:
  ProxyingURLLoaderFactory(
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

  mojo::BindingSet<network::mojom::URLLoaderFactory> proxy_bindings_;
  network::mojom::URLLoaderFactoryPtr target_factory_;

  base::WeakPtrFactory<ProxyingURLLoaderFactory> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(ProxyingURLLoaderFactory);
};

}  // namespace atom

#endif  // ATOM_BROWSER_NET_PROXYING_URL_LOADER_FACTORY_H_
