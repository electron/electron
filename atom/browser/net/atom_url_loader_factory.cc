// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/net/atom_url_loader_factory.h"

#include "services/network/public/cpp/url_loader_completion_status.h"
#include "services/network/public/mojom/url_loader.mojom.h"

namespace atom {

AtomURLLoaderFactory::AtomURLLoaderFactory() {}

AtomURLLoaderFactory::~AtomURLLoaderFactory() = default;

void AtomURLLoaderFactory::CreateLoaderAndStart(
    network::mojom::URLLoaderRequest loader,
    int32_t routing_id,
    int32_t request_id,
    uint32_t options,
    const network::ResourceRequest& request,
    network::mojom::URLLoaderClientPtr client,
    const net::MutableNetworkTrafficAnnotationTag& traffic_annotation) {
  client->OnComplete(network::URLLoaderCompletionStatus(net::ERR_FAILED));
}

void AtomURLLoaderFactory::Clone(
    network::mojom::URLLoaderFactoryRequest request) {
  bindings_.AddBinding(this, std::move(request));
}

}  // namespace atom
