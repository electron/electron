// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_EXTENSIONS_ATOM_EXTENSION_SYSTEM_FACTORY_H_
#define ATOM_BROWSER_EXTENSIONS_ATOM_EXTENSION_SYSTEM_FACTORY_H_

#include "atom/browser/extensions/atom_extension_system.h"
#include "base/memory/singleton.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"
#include "extensions/browser/extension_system_provider.h"

namespace extensions {
class ExtensionSystem;

class AtomExtensionSystemSharedFactory :
    public BrowserContextKeyedServiceFactory {
 public:
  static AtomExtensionSystem::Shared* GetForBrowserContext(
      content::BrowserContext* context);

  static AtomExtensionSystemSharedFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<AtomExtensionSystemSharedFactory>;

  AtomExtensionSystemSharedFactory();
  ~AtomExtensionSystemSharedFactory() override;

  // BrowserContextKeyedServiceFactory implementation:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;

  DISALLOW_COPY_AND_ASSIGN(AtomExtensionSystemSharedFactory);
};

// A factory that provides ShellExtensionSystem for app_shell.
class AtomExtensionSystemFactory : public ExtensionSystemProvider {
 public:
  // ExtensionSystemProvider implementation:
  ExtensionSystem* GetForBrowserContext(
      content::BrowserContext* context) override;

  static AtomExtensionSystemFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<AtomExtensionSystemFactory>;

  AtomExtensionSystemFactory();
  ~AtomExtensionSystemFactory() override;

  // BrowserContextKeyedServiceFactory implementation:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;
  bool ServiceIsCreatedWithBrowserContext() const override;

  DISALLOW_COPY_AND_ASSIGN(AtomExtensionSystemFactory);
};

}  // namespace extensions

#endif  // ATOM_BROWSER_EXTENSIONS_ATOM_EXTENSION_SYSTEM_FACTORY_H_
