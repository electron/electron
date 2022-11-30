// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/browser/extensions/electron_browser_context_keyed_service_factories.h"

#include "extensions/browser/updater/update_service_factory.h"
#include "shell/browser/extensions/electron_extension_system_factory.h"

namespace extensions::electron {

void EnsureBrowserContextKeyedServiceFactoriesBuilt() {
  // TODO(rockot): Remove this once UpdateService is supported across all
  // extensions embedders (and namely chrome.)
  UpdateServiceFactory::GetInstance();

  ElectronExtensionSystemFactory::GetInstance();
}

}  // namespace extensions::electron
