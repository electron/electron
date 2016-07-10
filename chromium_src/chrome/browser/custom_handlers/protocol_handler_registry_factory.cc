// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * This is a straight copy from chromium src with the exception that
 * the original context is returned below in GetBrowserContextToUse
 * and the atom_browser_context.h include was added.
 * This was last updated with a copy forked from 51.0.2704.106.
 */

#include "chrome/browser/custom_handlers/protocol_handler_registry_factory.h"
#include "brave/browser/brave_browser_context.h"

#include "base/memory/singleton.h"
#include "build/build_config.h"
#include "chrome/browser/custom_handlers/protocol_handler_registry.h"
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

content::BrowserContext* ProtocolHandlerRegistryFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return static_cast<brave::BraveBrowserContext*>(context)->original_context();
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
