// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/browser/extensions/atom_extension_system_factory.h"

#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "extensions/browser/extension_prefs_factory.h"
#include "extensions/browser/extension_registry_factory.h"
#include "shell/browser/extensions/atom_extension_system.h"

using content::BrowserContext;

namespace extensions {

ExtensionSystem* AtomExtensionSystemFactory::GetForBrowserContext(
    BrowserContext* context) {
  return static_cast<AtomExtensionSystem*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

// static
AtomExtensionSystemFactory* AtomExtensionSystemFactory::GetInstance() {
  return base::Singleton<AtomExtensionSystemFactory>::get();
}

AtomExtensionSystemFactory::AtomExtensionSystemFactory()
    : ExtensionSystemProvider("AtomExtensionSystem",
                              BrowserContextDependencyManager::GetInstance()) {
  DependsOn(ExtensionPrefsFactory::GetInstance());
  DependsOn(ExtensionRegistryFactory::GetInstance());
}

AtomExtensionSystemFactory::~AtomExtensionSystemFactory() {}

KeyedService* AtomExtensionSystemFactory::BuildServiceInstanceFor(
    BrowserContext* context) const {
  return new AtomExtensionSystem(context);
}

BrowserContext* AtomExtensionSystemFactory::GetBrowserContextToUse(
    BrowserContext* context) const {
  // Use a separate instance for incognito.
  return context;
}

bool AtomExtensionSystemFactory::ServiceIsCreatedWithBrowserContext() const {
  return true;
}

}  // namespace extensions
