// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "chrome/browser/profiles/profile_keyed_service_factory.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"

// Electron builds //chrome's SpellcheckServiceFactory, which derives from
// ProfileKeyedServiceFactory. Electron has one kind of BrowserContext and no
// Profile, so the factory keeps its ProfileSelections but always uses the
// context it is given.

ProfileKeyedServiceFactory::ProfileKeyedServiceFactory(const char* name)
    : ProfileKeyedServiceFactory(name, ProfileSelections::Builder().Build()) {}

ProfileKeyedServiceFactory::ProfileKeyedServiceFactory(
    const char* name,
    const ProfileSelections& profile_selections)
    : BrowserContextKeyedServiceFactory(
          name,
          BrowserContextDependencyManager::GetInstance()),
      profile_selections_(profile_selections) {}

ProfileKeyedServiceFactory::~ProfileKeyedServiceFactory() = default;

content::BrowserContext* ProfileKeyedServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return context;
}
