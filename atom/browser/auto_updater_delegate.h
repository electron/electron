// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_AUTO_UPDATER_DELEGATE_H_
#define ATOM_BROWSER_AUTO_UPDATER_DELEGATE_H_

#include <string>

#include "base/callback_forward.h"

namespace base {
class Time;
}

namespace auto_updater {

class AutoUpdaterDelegate {
 public:
  // An error happened.
  virtual void OnError(const std::string& error) {}

  // Checking to see if there is an update
  virtual void OnCheckingForUpdate() {}

  // There is an update available and it is being downloaded
  virtual void OnUpdateAvailable() {}

  // There is no available update.
  virtual void OnUpdateNotAvailable() {}

  // There is a new update which has been downloaded.
  virtual void OnUpdateDownloaded(const std::string& release_notes,
                                  const std::string& release_name,
                                  const base::Time& release_date,
                                  const std::string& update_url,
                                  const base::Closure& quit_and_install) {}

 protected:
  virtual ~AutoUpdaterDelegate() {}
};

}  // namespace auto_updater

#endif  // ATOM_BROWSER_AUTO_UPDATER_DELEGATE_H_
