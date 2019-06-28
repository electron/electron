// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/net/preconnect_manager_factory.h"

#include <memory>

#include "chrome/browser/predictors/preconnect_manager.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/storage_partition.h"

namespace {
class PreconnectManagerWrapper : public KeyedService {
 public:
  explicit PreconnectManagerWrapper(content::BrowserContext* context)
      : preconnect_manager_(
            std::make_unique<predictors::PreconnectManager>(nullptr, context)) {
  }
  predictors::PreconnectManager* GetPtr() { return preconnect_manager_.get(); }

 private:
  std::unique_ptr<predictors::PreconnectManager> preconnect_manager_;
};
}  // namespace

namespace electron {

// static
predictors::PreconnectManager* PreconnectManagerFactory::GetForContext(
    content::BrowserContext* context) {
  PreconnectManagerWrapper* ret = static_cast<PreconnectManagerWrapper*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
  return ret ? ret->GetPtr() : nullptr;
}

// static
PreconnectManagerFactory* PreconnectManagerFactory::GetInstance() {
  return base::Singleton<PreconnectManagerFactory>::get();
}

PreconnectManagerFactory::PreconnectManagerFactory()
    : BrowserContextKeyedServiceFactory(
          "PreconnectManager",
          BrowserContextDependencyManager::GetInstance()) {}

PreconnectManagerFactory::~PreconnectManagerFactory() {}

KeyedService* PreconnectManagerFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new PreconnectManagerWrapper(context);
}

}  // namespace electron
