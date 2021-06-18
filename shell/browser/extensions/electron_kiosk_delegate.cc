// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/browser/extensions/electron_kiosk_delegate.h"

namespace electron {

ElectronKioskDelegate::ElectronKioskDelegate() = default;

ElectronKioskDelegate::~ElectronKioskDelegate() = default;

bool ElectronKioskDelegate::IsAutoLaunchedKioskApp(
    const extensions::ExtensionId& id) const {
  return false;
}

}  // namespace electron
