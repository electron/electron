// Copyright (c) 2020 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <memory>
#include <utility>

#include "content/public/browser/non_network_url_loader_factory_base.h"  // nogncheck
#include "content/public/browser/web_contents.h"
#include "shell/browser/electron_browser_context.h"
#include "shell/browser/net/asar/asar_url_loader.h"
#include "shell/browser/protocol_registry.h"

namespace electron {

namespace {

// Provide support for accessing asar archives in file:// protocol.
class AsarURLLoaderFactory : public content::NonNetworkURLLoaderFactoryBase {
 public:
  static mojo::PendingRemote<network::mojom::URLLoaderFactory> Create() {
    mojo::PendingRemote<network::mojom::URLLoaderFactory> pending_remote;

    // The AsarURLLoaderFactory will delete itself when there are no more
    // receivers - see the NonNetworkURLLoaderFactoryBase::OnDisconnect method.
    new AsarURLLoaderFactory(pending_remote.InitWithNewPipeAndPassReceiver());

    return pending_remote;
  }

 private:
  AsarURLLoaderFactory(
      mojo::PendingReceiver<network::mojom::URLLoaderFactory> factory_receiver)
      : content::NonNetworkURLLoaderFactoryBase(std::move(factory_receiver)) {}
  ~AsarURLLoaderFactory() override = default;

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
    content::ContentBrowserClient::NonNetworkURLLoaderFactoryMap* factories,
    bool allow_file_access) {
  auto file_factory = factories->find(url::kFileScheme);
  if (file_factory != factories->end()) {
    // If Chromium already allows file access then replace the url factory to
    // also loading asar files.
    file_factory->second = AsarURLLoaderFactory::Create();
  } else if (allow_file_access) {
    // Otherwise only allow file access when it is explicitly allowed.
    //
    // Note that Chromium may call |emplace| to create the default file factory
    // after this call, it won't override our asar factory, but if asar support
    // breaks in future, please check if Chromium has changed the call.
    factories->emplace(url::kFileScheme, AsarURLLoaderFactory::Create());
  }

  for (const auto& it : handlers_) {
    factories->emplace(it.first, ElectronURLLoaderFactory::Create(
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
