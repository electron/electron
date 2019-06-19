// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/net/network_context_service_factory.h"

#include "atom/browser/net/network_context_service.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"

namespace atom {

NetworkContextService* NetworkContextServiceFactory::GetForContext(
    content::BrowserContext* browser_context) {
  return static_cast<NetworkContextService*>(
      GetInstance()->GetServiceForBrowserContext(browser_context, true));
}

NetworkContextServiceFactory* NetworkContextServiceFactory::GetInstance() {
  return base::Singleton<NetworkContextServiceFactory>::get();
}

NetworkContextServiceFactory::NetworkContextServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "ElectronNetworkContextService",
          BrowserContextDependencyManager::GetInstance()) {}

NetworkContextServiceFactory::~NetworkContextServiceFactory() {}

KeyedService* NetworkContextServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new NetworkContextService(static_cast<AtomBrowserContext*>(context));
}

content::BrowserContext* NetworkContextServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  // Create separate service for temporary sessions.
  return context;
}

}  // namespace atom
