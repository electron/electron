// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_NET_PRECONNECT_MANAGER_FACTORY_H_
#define ATOM_BROWSER_NET_PRECONNECT_MANAGER_FACTORY_H_

#include "base/macros.h"
#include "base/memory/singleton.h"
#include "base/memory/weak_ptr.h"
#include "chrome/browser/predictors/preconnect_manager.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

namespace atom {

class PreconnectManagerFactory : public BrowserContextKeyedServiceFactory {
 public:
  static predictors::PreconnectManager* GetForContext(
      content::BrowserContext* context);
  static PreconnectManagerFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<PreconnectManagerFactory>;

  PreconnectManagerFactory();
  ~PreconnectManagerFactory() override;

  // BrowserContextKeyedServiceFactory:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;

  mutable base::WeakPtrFactory<predictors::PreconnectManager::Delegate>
      weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(PreconnectManagerFactory);
};

}  // namespace atom

#endif  // ATOM_BROWSER_NET_PRECONNECT_MANAGER_FACTORY_H_
