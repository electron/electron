// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_HID_HID_CHOOSER_CONTEXT_FACTORY_H_
#define ELECTRON_SHELL_BROWSER_HID_HID_CHOOSER_CONTEXT_FACTORY_H_

#include <memory>

#include "base/no_destructor.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

namespace electron {

class HidChooserContext;

class HidChooserContextFactory : public BrowserContextKeyedServiceFactory {
 public:
  static HidChooserContext* GetForBrowserContext(
      content::BrowserContext* context);
  static HidChooserContext* GetForBrowserContextIfExists(
      content::BrowserContext* context);
  static HidChooserContextFactory* GetInstance();

  // disable copy
  HidChooserContextFactory(const HidChooserContextFactory&) = delete;
  HidChooserContextFactory& operator=(const HidChooserContextFactory&) = delete;

 private:
  friend base::NoDestructor<HidChooserContextFactory>;

  HidChooserContextFactory();
  ~HidChooserContextFactory() override;

  // BrowserContextKeyedServiceFactory:
  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      content::BrowserContext* profile) const override;
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_HID_HID_CHOOSER_CONTEXT_FACTORY_H_
