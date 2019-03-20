// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_SHELL_BROWSER_SHELL_BROWSER_CONTEXT_KEYED_SERVICE_FACTORIES_H_
#define EXTENSIONS_SHELL_BROWSER_SHELL_BROWSER_CONTEXT_KEYED_SERVICE_FACTORIES_H_

namespace extensions {
namespace shell {

// Ensures the existence of any BrowserContextKeyedServiceFactory provided by
// the core extensions code.
void EnsureBrowserContextKeyedServiceFactoriesBuilt();

}  // namespace shell
}  // namespace extensions

#endif  // EXTENSIONS_SHELL_BROWSER_SHELL_BROWSER_CONTEXT_KEYED_SERVICE_FACTORIES_H_
