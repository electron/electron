// Copyright (c) 2020 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <memory>
#include <utility>

#include "shell/browser/electron_browser_context.h"
#include "shell/browser/net/asar/asar_url_loader.h"
#include "shell/browser/protocol_registry.h"

namespace electron {

namespace {

// Provide support for accessing asar archives in file:// protocol.
class AsarURLLoaderFactory : public network::mojom::URLLoaderFactory {
 public:
  AsarURLLoaderFactory() {}

 private:
  // network::mojom::URLLoaderFactory:
  void CreateLoaderAndStart(
      mojo::PendingReceiver<network::mojom::URLLoader> loader,
      int32_t routing_id,
      int32_t request_id,
      uint32_t options,
      const network::ResourceRequest& request,
      mojo::PendingRemote<network::mojom::URLLoaderClient> client,
      const net::MutableNetworkTrafficAnnotationTag& traffic_annotation)
      override {
    asar::CreateAsarURLLoader(request, std::move(loader), std::move(client),
                              new net::HttpResponseHeaders(""));
  }

  void Clone(
      mojo::PendingReceiver<network::mojom::URLLoaderFactory> loader) override {
    receivers_.Add(this, std::move(loader));
  }

  mojo::ReceiverSet<network::mojom::URLLoaderFactory> receivers_;
};

}  // namespace

// static
ProtocolRegistry* ProtocolRegistry::FromBrowserContext(
    content::BrowserContext* context) {
  return static_cast<ElectronBrowserContext*>(context)->protocol_registry();
}

ProtocolRegistry::ProtocolRegistry() {}

ProtocolRegistry::~ProtocolRegistry() = default;

void ProtocolRegistry::RegisterURLLoaderFactories(
    URLLoaderFactoryType type,
    content::ContentBrowserClient::NonNetworkURLLoaderFactoryMap* factories) {
  // Override the default FileURLLoaderFactory to support asar archives.
  if (type == URLLoaderFactoryType::kNavigation) {
    // Always allow navigating to file:// URLs.
    //
    // Note that Chromium calls |emplace| to create the default file factory
    // after this call, so it won't override our asar factory.
    DCHECK(!base::Contains(*factories, url::kFileScheme));
    factories->emplace(url::kFileScheme,
                       std::make_unique<AsarURLLoaderFactory>());
  } else if (type == URLLoaderFactoryType::kDocumentSubResource) {
    // Only support requesting file:// subresource URLs when Chromium does so,
    // it is usually supported under file:// or about:blank documents.
    auto file_factory = factories->find(url::kFileScheme);
    if (file_factory != factories->end())
      file_factory->second = std::make_unique<AsarURLLoaderFactory>();
  }

  for (const auto& it : handlers_) {
    factories->emplace(it.first, std::make_unique<ElectronURLLoaderFactory>(
                                     it.second.first, it.second.second));
  }
}

bool ProtocolRegistry::RegisterProtocol(ProtocolType type,
                                        const std::string& scheme,
                                        const ProtocolHandler& handler) {
  return base::TryEmplace(handlers_, scheme, type, handler).second;
}

bool ProtocolRegistry::UnregisterProtocol(const std::string& scheme) {
  return handlers_.erase(scheme) != 0;
}

bool ProtocolRegistry::IsProtocolRegistered(const std::string& scheme) {
  return base::Contains(handlers_, scheme);
}

bool ProtocolRegistry::InterceptProtocol(ProtocolType type,
                                         const std::string& scheme,
                                         const ProtocolHandler& handler) {
  return base::TryEmplace(intercept_handlers_, scheme, type, handler).second;
}

bool ProtocolRegistry::UninterceptProtocol(const std::string& scheme) {
  return intercept_handlers_.erase(scheme) != 0;
}

bool ProtocolRegistry::IsProtocolIntercepted(const std::string& scheme) {
  return base::Contains(intercept_handlers_, scheme);
}

}  // namespace electron
