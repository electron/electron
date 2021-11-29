// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_EXTENSIONS_ELECTRON_DISPLAY_INFO_PROVIDER_H_
#define ELECTRON_SHELL_BROWSER_EXTENSIONS_ELECTRON_DISPLAY_INFO_PROVIDER_H_

#include "extensions/browser/api/system_display/display_info_provider.h"

namespace extensions {

class ElectronDisplayInfoProvider : public DisplayInfoProvider {
 public:
  ElectronDisplayInfoProvider();

  // disable copy
  ElectronDisplayInfoProvider(const ElectronDisplayInfoProvider&) = delete;
  ElectronDisplayInfoProvider& operator=(const ElectronDisplayInfoProvider&) =
      delete;
};

}  // namespace extensions

#endif  // ELECTRON_SHELL_BROWSER_EXTENSIONS_ELECTRON_DISPLAY_INFO_PROVIDER_H_
