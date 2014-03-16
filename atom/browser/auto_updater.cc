// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "atom/browser/auto_updater.h"

namespace auto_updater {

AutoUpdaterDelegate* AutoUpdater::delegate_ = NULL;

AutoUpdaterDelegate* AutoUpdater::GetDelegate() {
  return delegate_;
}

void AutoUpdater::SetDelegate(AutoUpdaterDelegate* delegate) {
  delegate_ = delegate;
}

}  // namespace auto_updater
