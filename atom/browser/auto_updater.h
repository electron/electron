// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_AUTO_UPDATER_H_
#define ATOM_BROWSER_AUTO_UPDATER_H_

#include <string>

#include "base/basictypes.h"

namespace auto_updater {

class AutoUpdaterDelegate;

class AutoUpdater {
 public:
  // Gets/Sets the delegate.
  static AutoUpdaterDelegate* GetDelegate();
  static void SetDelegate(AutoUpdaterDelegate* delegate);

  static void SetFeedURL(const std::string& url);
  static void CheckForUpdates();

 private:
  static AutoUpdaterDelegate* delegate_;

  DISALLOW_IMPLICIT_CONSTRUCTORS(AutoUpdater);
};

}  // namespace auto_updater

#endif  // ATOM_BROWSER_AUTO_UPDATER_H_
