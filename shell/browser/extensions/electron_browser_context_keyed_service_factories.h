// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_EXTENSIONS_ELECTRON_BROWSER_CONTEXT_KEYED_SERVICE_FACTORIES_H_
#define ELECTRON_SHELL_BROWSER_EXTENSIONS_ELECTRON_BROWSER_CONTEXT_KEYED_SERVICE_FACTORIES_H_

namespace extensions {
namespace electron {

// Ensures the existence of any BrowserContextKeyedServiceFactory provided by
// the core extensions code.
void EnsureBrowserContextKeyedServiceFactoriesBuilt();

}  // namespace electron
}  // namespace extensions

#endif  // ELECTRON_SHELL_BROWSER_EXTENSIONS_ELECTRON_BROWSER_CONTEXT_KEYED_SERVICE_FACTORIES_H_
