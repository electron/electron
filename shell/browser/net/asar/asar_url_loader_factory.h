// Copyright (c) 2021 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_NET_ASAR_ASAR_URL_LOADER_FACTORY_H_
#define SHELL_BROWSER_NET_ASAR_ASAR_URL_LOADER_FACTORY_H_

#include "content/public/browser/non_network_url_loader_factory_base.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"

namespace electron {

// Provide support for accessing asar archives in file:// protocol.
class AsarURLLoaderFactory : public content::NonNetworkURLLoaderFactoryBase {
 public:
  static mojo::PendingRemote<network::mojom::URLLoaderFactory> Create();

 private:
  AsarURLLoaderFactory(
      mojo::PendingReceiver<network::mojom::URLLoaderFactory> factory_receiver);
  ~AsarURLLoaderFactory() override;

  // network::mojom::URLLoaderFactory:
  void CreateLoaderAndStart(
      mojo::PendingReceiver<network::mojom::URLLoader> loader,
      int32_t routing_id,
      int32_t request_id,
      uint32_t options,
      const network::ResourceRequest& request,
      mojo::PendingRemote<network::mojom::URLLoaderClient> client,
      const net::MutableNetworkTrafficAnnotationTag& traffic_annotation)
      override;
};

}  // namespace electron

#endif  // SHELL_BROWSER_NET_ASAR_ASAR_URL_LOADER_FACTORY_H_
