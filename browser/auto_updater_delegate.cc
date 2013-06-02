// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/auto_updater_delegate.h"

#include "base/callback.h"

namespace auto_updater {

void AutoUpdaterDelegate::WillInstallUpdate(const std::string& version,
                                            const base::Closure& install) {
  install.Run();
}

}  // namespace auto_updater
