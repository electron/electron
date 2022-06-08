// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/auto_updater.h"

namespace auto_updater {

Delegate* AutoUpdater::delegate_ = nullptr;

Delegate* AutoUpdater::GetDelegate() {
  return delegate_;
}

void AutoUpdater::SetDelegate(Delegate* delegate) {
  delegate_ = delegate;
}

#if !BUILDFLAG(IS_MAC) || defined(MAS_BUILD)
std::string AutoUpdater::GetFeedURL() {
  return "";
}

void AutoUpdater::SetFeedURL(gin::Arguments* args) {}

void AutoUpdater::CheckForUpdates() {}

void AutoUpdater::QuitAndInstall() {}
#endif

}  // namespace auto_updater
