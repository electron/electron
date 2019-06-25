// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/net/proxying_url_loader_factory.h"

#include <utility>

#include "mojo/public/cpp/bindings/binding.h"
#include "services/network/public/mojom/url_loader.mojom.h"
#include "shell/browser/net/asar/asar_url_loader.h"

namespace electron {

ProxyingURLLoaderFactory::ProxyingURLLoaderFactory(
    const HandlersMap& handlers,
    network::mojom::URLLoaderFactoryRequest loader_request,
    network::mojom::URLLoaderFactoryPtrInfo target_factory_info)
    : handlers_(handlers) {
  target_factory_.Bind(std::move(target_factory_info));
  target_factory_.set_connection_error_handler(base::BindOnce(
      &ProxyingURLLoaderFactory::OnTargetFactoryError, base::Unretained(this)));
  proxy_bindings_.AddBinding(this, std::move(loader_request));
  proxy_bindings_.set_connection_error_handler(base::BindRepeating(
      &ProxyingURLLoaderFactory::OnProxyBindingError, base::Unretained(this)));
}

ProxyingURLLoaderFactory::~ProxyingURLLoaderFactory() = default;

void ProxyingURLLoaderFactory::CreateLoaderAndStart(
    network::mojom::URLLoaderRequest loader,
    int32_t routing_id,
    int32_t request_id,
    uint32_t options,
    const network::ResourceRequest& request,
    network::mojom::URLLoaderClientPtr client,
    const net::MutableNetworkTrafficAnnotationTag& traffic_annotation) {
  // Check if user has intercepted this scheme.
  auto it = handlers_.find(request.url.scheme());
  if (it != handlers_.end()) {
    // <scheme, <type, handler>>
    it->second.second.Run(
        request, base::BindOnce(&AtomURLLoaderFactory::StartLoading,
                                std::move(loader), routing_id, request_id,
                                options, request, std::move(client),
                                traffic_annotation, this, it->second.first));
    return;
  }

  // Intercept file:// protocol to support asar archives.
  if (request.url.SchemeIsFile()) {
    asar::CreateAsarURLLoader(request, std::move(loader), std::move(client),
                              nullptr);
    return;
  }

  // Pass-through to the original factory.
  target_factory_->CreateLoaderAndStart(std::move(loader), routing_id,
                                        request_id, options, request,
                                        std::move(client), traffic_annotation);
}

void ProxyingURLLoaderFactory::Clone(
    network::mojom::URLLoaderFactoryRequest loader_request) {
  proxy_bindings_.AddBinding(this, std::move(loader_request));
}

void ProxyingURLLoaderFactory::OnTargetFactoryError() {
  delete this;
}

void ProxyingURLLoaderFactory::OnProxyBindingError() {
  if (proxy_bindings_.empty())
    delete this;
}

}  // namespace electron
