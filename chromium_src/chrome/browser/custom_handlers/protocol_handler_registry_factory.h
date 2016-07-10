// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * This is a straight copy from chromium src
 * This was last updated with a copy forked from 51.0.2704.106.
 */

#ifndef CHROME_BROWSER_CUSTOM_HANDLERS_PROTOCOL_HANDLER_REGISTRY_FACTORY_H_
#define CHROME_BROWSER_CUSTOM_HANDLERS_PROTOCOL_HANDLER_REGISTRY_FACTORY_H_

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

class Profile;
class ProtocolHandlerRegistry;

namespace base {
template <typename T> struct DefaultSingletonTraits;
}

// Singleton that owns all ProtocolHandlerRegistrys and associates them with
// Profiles. Listens for the Profile's destruction notification and cleans up
// the associated ProtocolHandlerRegistry.
class ProtocolHandlerRegistryFactory
    : public BrowserContextKeyedServiceFactory {
 public:
  // Returns the singleton instance of the ProtocolHandlerRegistryFactory.
  static ProtocolHandlerRegistryFactory* GetInstance();

  // Returns the ProtocolHandlerRegistry that provides intent registration for
  // |context|. Ownership stays with this factory object.
  static ProtocolHandlerRegistry* GetForBrowserContext(
      content::BrowserContext* context);

 protected:
  // BrowserContextKeyedServiceFactory implementation.
  bool ServiceIsCreatedWithBrowserContext() const override;
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;
  bool ServiceIsNULLWhileTesting() const override;

 private:
  friend struct base::DefaultSingletonTraits<ProtocolHandlerRegistryFactory>;

  ProtocolHandlerRegistryFactory();
  ~ProtocolHandlerRegistryFactory() override;

  // BrowserContextKeyedServiceFactory implementation.
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* profile) const override;

  DISALLOW_COPY_AND_ASSIGN(ProtocolHandlerRegistryFactory);
};

#endif  // CHROME_BROWSER_CUSTOM_HANDLERS_PROTOCOL_HANDLER_REGISTRY_FACTORY_H_
