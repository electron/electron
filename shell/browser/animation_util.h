// Copyright (c) 2022 Salesforce, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_ANIMATION_UTIL_H_
#define ELECTRON_SHELL_BROWSER_ANIMATION_UTIL_H_

#include "build/build_config.h"

#if BUILDFLAG(IS_MAC)
class ScopedCAActionDisabler {
 public:
  ScopedCAActionDisabler();
  ~ScopedCAActionDisabler();
};
#endif

#endif  // ELECTRON_SHELL_BROWSER_ANIMATION_UTIL_H_
