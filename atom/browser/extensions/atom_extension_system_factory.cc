// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/extensions/atom_extension_system_factory.h"

#include "brave/browser/brave_browser_context.h"
#include "content/public/browser/browser_context.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "extensions/browser/declarative_user_script_manager_factory.h"
#include "extensions/browser/event_router_factory.h"
#include "extensions/browser/extension_prefs_factory.h"
#include "extensions/browser/extension_registry_factory.h"
#include "extensions/browser/extension_system.h"
#include "extensions/browser/extensions_browser_client.h"
#include "extensions/browser/process_manager_factory.h"
#include "extensions/browser/renderer_startup_helper.h"

namespace extensions {

// AtomExtensionSystemSharedFactory

// static
AtomExtensionSystem::Shared*
AtomExtensionSystemSharedFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  return static_cast<AtomExtensionSystem::Shared*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

// static
AtomExtensionSystemSharedFactory*
    AtomExtensionSystemSharedFactory::GetInstance() {
  return base::Singleton<AtomExtensionSystemSharedFactory>::get();
}

AtomExtensionSystemSharedFactory::AtomExtensionSystemSharedFactory()
    : BrowserContextKeyedServiceFactory(
        "ExtensionSystemShared",
        BrowserContextDependencyManager::GetInstance()) {
  DependsOn(ExtensionPrefsFactory::GetInstance());
  // This depends on ExtensionService which depends on ExtensionRegistry.
  DependsOn(ExtensionRegistryFactory::GetInstance());
  DependsOn(ProcessManagerFactory::GetInstance());
  DependsOn(RendererStartupHelperFactory::GetInstance());
  DependsOn(DeclarativeUserScriptManagerFactory::GetInstance());
  DependsOn(EventRouterFactory::GetInstance());
}

AtomExtensionSystemSharedFactory::~AtomExtensionSystemSharedFactory() {
}

KeyedService* AtomExtensionSystemSharedFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new AtomExtensionSystem::Shared(
      static_cast<brave::BraveBrowserContext*>(context));
}

content::BrowserContext*
    AtomExtensionSystemSharedFactory::GetBrowserContextToUse(
                              content::BrowserContext* context) const {
  // Redirected in incognito.
  return ExtensionsBrowserClient::Get()->GetOriginalContext(context);
}

// ExtensionSystemFactory
ExtensionSystem* AtomExtensionSystemFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  return static_cast<AtomExtensionSystem*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

// static
AtomExtensionSystemFactory* AtomExtensionSystemFactory::GetInstance() {
  return base::Singleton<AtomExtensionSystemFactory>::get();
}

AtomExtensionSystemFactory::AtomExtensionSystemFactory()
    : ExtensionSystemProvider("ExtensionSystem",
                              BrowserContextDependencyManager::GetInstance()) {
  DCHECK(ExtensionsBrowserClient::Get())
      << "ExtensionSystemFactory must be initialized after BrowserProcess";
  DependsOn(AtomExtensionSystemSharedFactory::GetInstance());
}

AtomExtensionSystemFactory::~AtomExtensionSystemFactory() {
}

KeyedService* AtomExtensionSystemFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new AtomExtensionSystem(
      static_cast<brave::BraveBrowserContext*>(context));
}

content::BrowserContext* AtomExtensionSystemFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  // Use a separate instance for incognito.
  return context;
}

bool AtomExtensionSystemFactory::ServiceIsCreatedWithBrowserContext() const {
  return true;
}

}  // namespace extensions
