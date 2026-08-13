// Copyright (c) 2021 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/net/asar/asar_url_loader_factory.h"

#include <utility>

#include "base/memory/self_deleting.h"
#include "shell/browser/net/asar/asar_url_loader.h"

namespace electron {

// static
mojo::PendingRemote<network::mojom::URLLoaderFactory>
AsarURLLoaderFactory::Create() {
  mojo::PendingRemote<network::mojom::URLLoaderFactory> pending_remote;

  // The AsarURLLoaderFactory will delete itself when there are no more
  // receivers - see the SelfDeletingURLLoaderFactory::OnDisconnect method.
  base::MakeSelfDeleting<AsarURLLoaderFactory>(
      pending_remote.InitWithNewPipeAndPassReceiver());

  return pending_remote;
}

AsarURLLoaderFactory::AsarURLLoaderFactory(
    mojo::PendingReceiver<network::mojom::URLLoaderFactory> factory_receiver,
    base::SelfDeletingPassKey key)
    : network::SelfDeletingURLLoaderFactory(std::move(factory_receiver), key) {}
AsarURLLoaderFactory::~AsarURLLoaderFactory() = default;

void AsarURLLoaderFactory::CreateLoaderAndStart(
    mojo::PendingReceiver<network::mojom::URLLoader> loader,
    int32_t request_id,
    uint32_t options,
    const network::ResourceRequest& request,
    mojo::PendingRemote<network::mojom::URLLoaderClient> client,
    const net::MutableNetworkTrafficAnnotationTag& traffic_annotation) {
  asar::CreateAsarURLLoader(request, std::move(loader), std::move(client),
                            new net::HttpResponseHeaders(""));
}

}  // namespace electron
