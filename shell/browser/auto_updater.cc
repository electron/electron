// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/auto_updater.h"

#include "base/no_destructor.h"
#include "build/build_config.h"
#include "electron/mas.h"

namespace auto_updater {

Delegate* AutoUpdater::delegate_ = nullptr;

Delegate* AutoUpdater::GetDelegate() {
  return delegate_;
}

void AutoUpdater::SetDelegate(Delegate* delegate) {
  delegate_ = delegate;
}

std::string& AutoUpdater::GetFeedURL() {
  static base::NoDestructor<std::string> feed_url;
  return *feed_url;
}

#if !BUILDFLAG(IS_MAC) || IS_MAS_BUILD()
void AutoUpdater::SetFeedURL(gin::Arguments* args) {}

void AutoUpdater::CheckForUpdates() {}

void AutoUpdater::QuitAndInstall() {}

bool AutoUpdater::IsVersionAllowedForUpdate(const std::string& current_version,
                                            const std::string& target_version) {
  return false;
}
#endif

}  // namespace auto_updater
