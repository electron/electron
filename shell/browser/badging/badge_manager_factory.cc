// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/browser/badging/badge_manager_factory.h"

#include "base/memory/ptr_util.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "shell/browser/badging/badge_manager.h"

namespace badging {

// static
BadgeManager* BadgeManagerFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  return static_cast<badging::BadgeManager*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

// static
BadgeManagerFactory* BadgeManagerFactory::GetInstance() {
  static base::NoDestructor<BadgeManagerFactory> instance;
  return instance.get();
}

BadgeManagerFactory::BadgeManagerFactory()
    : BrowserContextKeyedServiceFactory(
          "BadgeManager",
          BrowserContextDependencyManager::GetInstance()) {}

BadgeManagerFactory::~BadgeManagerFactory() = default;

std::unique_ptr<KeyedService>
BadgeManagerFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  return std::make_unique<BadgeManager>();
}

}  // namespace badging
