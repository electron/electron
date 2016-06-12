// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#pragma once

#include <string>

#include "base/macros.h"
#include "build/build_config.h"

namespace base {
class Time;
}

namespace auto_updater {

class Delegate {
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
                                  const std::string& update_url) {}

 protected:
  virtual ~Delegate() {}
};

class AutoUpdater {
 public:
  // Gets/Sets the delegate.
  static Delegate* GetDelegate();
  static void SetDelegate(Delegate* delegate);

  static void SetFeedURL(const std::string& url);
  static void CheckForUpdates();
  static void QuitAndInstall();

 private:
  static Delegate* delegate_;

  DISALLOW_IMPLICIT_CONSTRUCTORS(AutoUpdater);
};

}  // namespace auto_updater
