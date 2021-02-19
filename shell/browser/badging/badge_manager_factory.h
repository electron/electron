// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_BADGING_BADGE_MANAGER_FACTORY_H_
#define SHELL_BROWSER_BADGING_BADGE_MANAGER_FACTORY_H_

#include "base/macros.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

namespace base {
template <typename T>
struct DefaultSingletonTraits;
}

namespace badging {

class BadgeManager;

// Singleton that provides access to context specific BadgeManagers.
class BadgeManagerFactory : public BrowserContextKeyedServiceFactory {
 public:
  // Gets the BadgeManager for the specified context
  static BadgeManager* GetForBrowserContext(content::BrowserContext* context);

  // Returns the BadgeManagerFactory singleton.
  static BadgeManagerFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<BadgeManagerFactory>;

  BadgeManagerFactory();
  ~BadgeManagerFactory() override;

  // BrowserContextKeyedServiceFactory
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;

  DISALLOW_COPY_AND_ASSIGN(BadgeManagerFactory);
};

}  // namespace badging

#endif  // SHELL_BROWSER_BADGING_BADGE_MANAGER_FACTORY_H_
