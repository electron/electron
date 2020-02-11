// Copyright (c) 2019 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_EXTENSIONS_ELECTRON_EXTENSIONS_BROWSER_API_PROVIDER_H_
#define SHELL_BROWSER_EXTENSIONS_ELECTRON_EXTENSIONS_BROWSER_API_PROVIDER_H_

#include "base/macros.h"
#include "extensions/browser/extensions_browser_api_provider.h"

namespace extensions {

class ElectronExtensionsBrowserAPIProvider
    : public ExtensionsBrowserAPIProvider {
 public:
  ElectronExtensionsBrowserAPIProvider();
  ~ElectronExtensionsBrowserAPIProvider() override;

  void RegisterExtensionFunctions(ExtensionFunctionRegistry* registry) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(ElectronExtensionsBrowserAPIProvider);
};

}  // namespace extensions

#endif  // SHELL_BROWSER_EXTENSIONS_ELECTRON_EXTENSIONS_BROWSER_API_PROVIDER_H_
