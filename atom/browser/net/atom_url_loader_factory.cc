// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/net/atom_url_loader_factory.h"

#include <string>
#include <utility>

#include "content/public/browser/browser_thread.h"
#include "services/network/public/cpp/url_loader_completion_status.h"
#include "services/network/public/mojom/url_loader.mojom.h"

using content::BrowserThread;

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
  std::string contents = "Not Implemented";

  uint32_t size = base::saturated_cast<uint32_t>(contents.size());
  mojo::DataPipe pipe(size);
  MojoResult result = pipe.producer_handle->WriteData(
      contents.data(), &size, MOJO_WRITE_DATA_FLAG_NONE);
  if (result != MOJO_RESULT_OK || size < contents.size()) {
    client->OnComplete(network::URLLoaderCompletionStatus(net::ERR_FAILED));
    return;
  }

  network::ResourceResponseHead head;
  head.mime_type = "text/html";
  head.charset = "utf-8";
  client->OnReceiveResponse(head);
  client->OnStartLoadingResponseBody(std::move(pipe.consumer_handle));
  client->OnComplete(network::URLLoaderCompletionStatus(net::OK));
}

void AtomURLLoaderFactory::Clone(
    network::mojom::URLLoaderFactoryRequest request) {
  bindings_.AddBinding(this, std::move(request));
}

}  // namespace atom
