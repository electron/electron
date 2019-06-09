// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
#include <memory>

#include "atom/browser/net/preconnect_manager_factory.h"

#include "chrome/browser/predictors/preconnect_manager.h"
#include "chrome/browser/profiles/profile.h"
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
      Profile* profile)
      : ptr_(new predictors::PreconnectManager(delegate, profile)) {
    ptr_->SetNetworkContextForTesting(
        content::BrowserContext::GetDefaultStoragePartition(profile)
            ->GetNetworkContext());
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

namespace atom {

// static
predictors::PreconnectManager* PreconnectManagerFactory::GetForProfile(
    Profile* profile) {
  return static_cast<PreconnectManagerWrapper*>(
             GetInstance()->GetServiceForBrowserContext(profile, true))
      ->GetPtr();
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
  Profile* profile = static_cast<Profile*>(context);
  return new PreconnectManagerWrapper(weak_factory_.GetWeakPtr(), profile);
}

}  // namespace atom
