// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_EXTENSIONS_ELECTRON_EXTENSION_INFO_H_
#define ELECTRON_SHELL_BROWSER_EXTENSIONS_ELECTRON_EXTENSION_INFO_H_

#include "base/memory/raw_ptr.h"

namespace electron {
class ElectronBrowserContext;
}

namespace extensions {

class Extension;

struct ElectronExtensionInfo {
  explicit ElectronExtensionInfo(
      const Extension* extension_in,
      const electron::ElectronBrowserContext* browser_context_in)
      : extension(extension_in), browser_context(browser_context_in) {
    DCHECK(extension_in);
    DCHECK(browser_context_in);
  }

  raw_ptr<const Extension> extension;
  raw_ptr<const electron::ElectronBrowserContext> browser_context;
};

}  // namespace extensions

#endif  // ELECTRON_SHELL_BROWSER_EXTENSIONS_ELECTRON_EXTENSION_INFO_H_
