// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/browser/extensions/electron_browser_context_keyed_service_factories.h"

#include "extensions/browser/api/cookies/cookies_api.h"
#include "extensions/browser/manifest_v2_handler.h"
#include "extensions/browser/updater/update_service_factory.h"
#include "shell/browser/extensions/electron_extension_system_factory.h"

namespace extensions::electron {

void EnsureBrowserContextKeyedServiceFactoriesBuilt() {
  // TODO(rockot): Remove this once UpdateService is supported across all
  // extensions embedders (and namely chrome.)
  UpdateServiceFactory::GetInstance();

  // Dispatches chrome.cookies.onChanged. Embedders build this factory; it is
  // not part of the //extensions list.
  CookiesAPI::GetFactoryInstance();

  // Chromium disables Manifest V2 extensions when this service starts. It is
  // created on first use unless its factory is built, and extension functions
  // do use it, so build it here: the service then starts with the session,
  // before an app can have loaded any extension.
  ManifestV2Handler::GetFactory();

  ElectronExtensionSystemFactory::GetInstance();
}

}  // namespace extensions::electron
