// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_SESSION_PREFERENCES_H_
#define ELECTRON_SHELL_BROWSER_SESSION_PREFERENCES_H_

#include <vector>

#include "base/files/file_path.h"
#include "base/supports_user_data.h"
#include "shell/browser/preload_script.h"

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

  bool HasServiceWorkerPreloadScript();

  // Bumped whenever the preload script registry changes; used to skip
  // redundant RendererStartupData pushes.
  uint64_t preload_generation() const { return preload_generation_; }
  void BumpPreloadGeneration() { ++preload_generation_; }

 private:
  SessionPreferences();

  // The user data key.
  static int kLocatorKey;

  std::vector<PreloadScript> preload_scripts_;
  uint64_t preload_generation_ = 0;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_SESSION_PREFERENCES_H_
