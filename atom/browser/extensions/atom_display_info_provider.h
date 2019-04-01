// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_EXTENSIONS_SHELL_DISPLAY_INFO_PROVIDER_H_
#define ATOM_BROWSER_EXTENSIONS_SHELL_DISPLAY_INFO_PROVIDER_H_

#include "base/macros.h"
#include "extensions/browser/api/system_display/display_info_provider.h"

namespace extensions {

class ShellDisplayInfoProvider : public DisplayInfoProvider {
 public:
  ShellDisplayInfoProvider();

 private:
  DISALLOW_COPY_AND_ASSIGN(ShellDisplayInfoProvider);
};

}  // namespace extensions

#endif  // ATOM_BROWSER_EXTENSIONS_SHELL_DISPLAY_INFO_PROVIDER_H_
