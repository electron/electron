// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_AUTO_UPDATER_DELEGATE_H_
#define ATOM_BROWSER_AUTO_UPDATER_DELEGATE_H_

#include <string>

#include "base/callback_forward.h"

namespace auto_updater {

class AutoUpdaterDelegate {
 public:
  virtual void WillInstallUpdate(const std::string& version,
                                 const base::Closure& install);

 protected:
  virtual ~AutoUpdaterDelegate() {}
};

}  // namespace auto_updater

#endif  // ATOM_BROWSER_AUTO_UPDATER_DELEGATE_H_
