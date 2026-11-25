// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_SESSION_PREFERENCES_H_
#define ELECTRON_SHELL_BROWSER_SESSION_PREFERENCES_H_

#include <vector>

#include "base/supports_user_data.h"
#include "shell/browser/preload_script.h"

class GURL;

namespace content {
class BrowserContext;
}

namespace electron {

class SessionPreferences : public base::SupportsUserData::Data {
 public:
  static SessionPreferences* FromBrowserContext(
      content::BrowserContext* context);

  static void CreateForBrowserContext(content::BrowserContext* context);

  ~SessionPreferences() override;

  std::vector<PreloadScript>& preload_scripts() { return preload_scripts_; }

  // Whether any service worker preload's URL filters match |scope|.
  bool HasServiceWorkerPreloadScript(const GURL& scope);

  // Whether a frame preload with allFrames was ever registered. Not reset on
  // unregister, so that a subframe which ran one is still sent the (now
  // empty) set of preloads for its next document.
  bool all_frames_preload_registered() const {
    return all_frames_preload_registered_;
  }
  void set_all_frames_preload_registered() {
    all_frames_preload_registered_ = true;
  }

 private:
  SessionPreferences();

  // The user data key.
  static int kLocatorKey;

  std::vector<PreloadScript> preload_scripts_;
  bool all_frames_preload_registered_ = false;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_SESSION_PREFERENCES_H_
