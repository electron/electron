// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_SHELL_BROWSER_SHELL_EXTENSION_SYSTEM_FACTORY_H_
#define EXTENSIONS_SHELL_BROWSER_SHELL_EXTENSION_SYSTEM_FACTORY_H_

#include "base/macros.h"
#include "base/memory/singleton.h"
#include "extensions/browser/extension_system_provider.h"

namespace extensions {

// A factory that provides ShellExtensionSystem for app_shell.
class ShellExtensionSystemFactory : public ExtensionSystemProvider {
 public:
  // ExtensionSystemProvider implementation:
  ExtensionSystem* GetForBrowserContext(
      content::BrowserContext* context) override;

  static ShellExtensionSystemFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<ShellExtensionSystemFactory>;

  ShellExtensionSystemFactory();
  ~ShellExtensionSystemFactory() override;

  // BrowserContextKeyedServiceFactory implementation:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;
  bool ServiceIsCreatedWithBrowserContext() const override;

  DISALLOW_COPY_AND_ASSIGN(ShellExtensionSystemFactory);
};

}  // namespace extensions

#endif  // EXTENSIONS_SHELL_BROWSER_SHELL_EXTENSION_SYSTEM_FACTORY_H_
