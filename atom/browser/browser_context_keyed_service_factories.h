// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_BROWSER_CONTEXT_KEYED_SERVICE_FACTORIES_H_
#define ATOM_BROWSER_BROWSER_CONTEXT_KEYED_SERVICE_FACTORIES_H_

namespace atom {

// Ensures the existence of any BrowserContextKeyedServiceFactory
void EnsureBrowserContextKeyedServiceFactoriesBuilt();

}  // namespace atom

#endif  // ATOM_BROWSER_BROWSER_CONTEXT_KEYED_SERVICE_FACTORIES_H_
