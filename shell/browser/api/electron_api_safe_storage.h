// Copyright (c) 2021 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_API_ELECTRON_API_SAFE_STORAGE_H_
#define ELECTRON_SHELL_BROWSER_API_ELECTRON_API_SAFE_STORAGE_H_

#include "base/dcheck_is_on.h"

namespace electron {

namespace safestorage {

// Used in a DCHECK to validate that our assumption that the network context
// manager has initialized before app ready holds true. Only used in the
// testing build
#if DCHECK_IS_ON()
void SetElectronCryptoReady(bool ready);
#endif

}  // namespace safestorage

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_API_ELECTRON_API_SAFE_STORAGE_H_
