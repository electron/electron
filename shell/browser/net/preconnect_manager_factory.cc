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
class StubDelegate : public predictors::PreconnectManager::Delegate {
 public:
  void PreconnectFinished(
      std::unique_ptr<predictors::PreconnectStats> stats) override {}
};

class PreconnectManagerWrapper : public KeyedService {
 public:
  PreconnectManagerWrapper(
      base::WeakPtr<predictors::PreconnectManager::Delegate> delegate,
      content::BrowserContext* context)
      : ptr_(new predictors::PreconnectManager(
            delegate,
            reinterpret_cast<Profile*>(context))) {
    if (context) {
      ptr_->SetNetworkContextForTesting(
          content::BrowserContext::GetDefaultStoragePartition(context)
              ->GetNetworkContext());
    }
  }
  ~PreconnectManagerWrapper() override {
    delete ptr_;
    ptr_ = NULL;
  }
  predictors::PreconnectManager* GetPtr() { return ptr_; }

 private:
  predictors::PreconnectManager* ptr_;
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
          BrowserContextDependencyManager::GetInstance()),
      weak_factory_(new StubDelegate()) {}

PreconnectManagerFactory::~PreconnectManagerFactory() {}

KeyedService* PreconnectManagerFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new PreconnectManagerWrapper(weak_factory_.GetWeakPtr(), context);
}

}  // namespace electron
