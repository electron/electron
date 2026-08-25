// Copyright (c) 2021 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/net/asar/asar_url_loader_factory.h"

#include <utility>

#include "base/functional/bind.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "shell/browser/net/asar/asar_url_loader.h"

namespace electron {

// static
mojo::PendingRemote<network::mojom::URLLoaderFactory>
AsarURLLoaderFactory::Create() {
  mojo::PendingRemote<network::mojom::URLLoaderFactory> pending_remote;
  // Bound on the IO thread rather than the UI thread it is created from: all
  // it does is post loaders to the thread pool.
  content::GetIOThreadTaskRunner({})->PostTask(
      FROM_HERE,
      base::BindOnce(&AsarURLLoaderFactory::BindOnIOThread,
                     pending_remote.InitWithNewPipeAndPassReceiver()));
  return pending_remote;
}

// static
void AsarURLLoaderFactory::BindOnIOThread(
    mojo::PendingReceiver<network::mojom::URLLoaderFactory> receiver) {
  // Deletes itself with its last receiver, see
  // SelfDeletingURLLoaderFactory::OnDisconnect.
  new AsarURLLoaderFactory(std::move(receiver));
}  // NOLINT(clang-analyzer-cplusplus.NewDeleteLeaks)

AsarURLLoaderFactory::AsarURLLoaderFactory(
    mojo::PendingReceiver<network::mojom::URLLoaderFactory> factory_receiver)
    : network::SelfDeletingURLLoaderFactory(std::move(factory_receiver)) {}
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
