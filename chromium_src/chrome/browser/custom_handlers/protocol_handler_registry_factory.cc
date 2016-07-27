// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/custom_handlers/protocol_handler_registry_factory.h"

#include "base/memory/singleton.h"
#include "build/build_config.h"
#include "chrome/browser/custom_handlers/protocol_handler_registry.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"

// static
ProtocolHandlerRegistryFactory* ProtocolHandlerRegistryFactory::GetInstance() {
  return base::Singleton<ProtocolHandlerRegistryFactory>::get();
}

// static
ProtocolHandlerRegistry* ProtocolHandlerRegistryFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  return static_cast<ProtocolHandlerRegistry*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

ProtocolHandlerRegistryFactory::ProtocolHandlerRegistryFactory()
    : BrowserContextKeyedServiceFactory(
        "ProtocolHandlerRegistry",
        BrowserContextDependencyManager::GetInstance()) {
}

ProtocolHandlerRegistryFactory::~ProtocolHandlerRegistryFactory() {
}

// Will be created when initializing profile_io_data, so we might
// as well have the framework create this along with other
// PKSs to preserve orderly civic conduct :)
bool
ProtocolHandlerRegistryFactory::ServiceIsCreatedWithBrowserContext() const {
  return true;
}

// Allows the produced registry to be used in incognito mode.
content::BrowserContext* ProtocolHandlerRegistryFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}

// Do not create this service for tests. MANY tests will fail
// due to the threading requirements of this service. ALSO,
// not creating this increases test isolation (which is GOOD!)
bool ProtocolHandlerRegistryFactory::ServiceIsNULLWhileTesting() const {
  return true;
}

KeyedService* ProtocolHandlerRegistryFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  ProtocolHandlerRegistry* registry = new ProtocolHandlerRegistry(
      context, new ProtocolHandlerRegistry::Delegate());

#if defined(OS_CHROMEOS)
  // If installing defaults, they must be installed prior calling
  // InitProtocolSettings
  registry->InstallDefaultsForChromeOS();
#endif

  // Must be called as a part of the creation process.
  registry->InitProtocolSettings();

  return registry;
}
