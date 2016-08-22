// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_AUTOFILL_PERSONAL_DATA_MANAGER_FACTORY_H_
#define ATOM_BROWSER_AUTOFILL_PERSONAL_DATA_MANAGER_FACTORY_H_

#include "base/compiler_specific.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"
#include "components/keyed_service/core/keyed_service.h"

namespace base {
template <typename T> struct DefaultSingletonTraits;
}

class Profile;

namespace autofill {

class PersonalDataManager;

// Singleton that owns all PersonalDataManagers and associates them with
// Profiles.
// Listens for the Profile's destruction notification and cleans up the
// associated PersonalDataManager.
class PersonalDataManagerFactory : public BrowserContextKeyedServiceFactory {
 public:
  static PersonalDataManager* GetForBrowserContext(
                             content::BrowserContext* context);

  static PersonalDataManagerFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<PersonalDataManagerFactory>;

  PersonalDataManagerFactory();
  ~PersonalDataManagerFactory() override;

  // BrowserContextKeyedServiceFactory:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* profile) const override;
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;
};

}  // namespace autofill

#endif  // ATOM_BROWSER_AUTOFILL_PERSONAL_DATA_MANAGER_FACTORY_H_
