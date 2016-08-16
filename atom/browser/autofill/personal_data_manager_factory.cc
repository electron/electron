// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/autofill/personal_data_manager_factory.h"

#include "base/memory/singleton.h"
#include "brave/browser/brave_browser_context.h"
#include "brave/browser/brave_content_browser_client.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "components/autofill/core/browser/personal_data_manager.h"
#include "components/autofill/core/browser/webdata/autofill_webdata_service.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/user_prefs/user_prefs.h"

namespace autofill {

// static
PersonalDataManager* PersonalDataManagerFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  return static_cast<PersonalDataManager*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

// static
PersonalDataManagerFactory* PersonalDataManagerFactory::GetInstance() {
  return base::Singleton<PersonalDataManagerFactory>::get();
}

PersonalDataManagerFactory::PersonalDataManagerFactory()
    : BrowserContextKeyedServiceFactory(
        "PersonalDataManager",
        BrowserContextDependencyManager::GetInstance()) {
}

PersonalDataManagerFactory::~PersonalDataManagerFactory() {
}

KeyedService* PersonalDataManagerFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  PersonalDataManager* service =
      new PersonalDataManager(
          brave::BraveContentBrowserClient::Get()->GetApplicationLocale());
  service->Init(static_cast<brave::BraveBrowserContext*>(context)
                ->GetAutofillWebdataService(),
                user_prefs::UserPrefs::Get(context),
                nullptr,
                nullptr,
                context->IsOffTheRecord());
  return service;
}

content::BrowserContext* PersonalDataManagerFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}

}  // namespace autofill
