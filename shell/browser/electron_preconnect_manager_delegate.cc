// Copyright (c) 2025 Microsoft, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/electron_preconnect_manager_delegate.h"

namespace electron {

ElectronPreconnectManagerDelegate::ElectronPreconnectManagerDelegate() =
    default;
ElectronPreconnectManagerDelegate::~ElectronPreconnectManagerDelegate() =
    default;

bool ElectronPreconnectManagerDelegate::IsPreconnectEnabled() {
  return true;
}

}  // namespace electron
