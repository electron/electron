// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_SESSION_PREFERENCES_H_
#define ELECTRON_SHELL_BROWSER_SESSION_PREFERENCES_H_

#include <vector>

#include "base/files/file_path.h"
#include "base/memory/weak_ptr.h"
#include "base/supports_user_data.h"
#include "shell/browser/api/electron_api_utility_process.h"
#include "shell/browser/preload_script.h"

namespace content {
class BrowserContext;
}

namespace electron {

namespace api {
class UtilityProcessWrapper;
}

class SessionPreferences : public base::SupportsUserData::Data {
 public:
  static SessionPreferences* FromBrowserContext(
      content::BrowserContext* context);

  static void CreateForBrowserContext(content::BrowserContext* context);

  ~SessionPreferences() override;

  std::vector<PreloadScript>& preload_scripts() { return preload_scripts_; }

  bool HasServiceWorkerPreloadScript();

  const base::WeakPtr<api::UtilityProcessWrapper>& GetLocalAIHandler() const {
    return local_ai_handler_;
  }

  void SetLocalAIHandler(base::WeakPtr<api::UtilityProcessWrapper> handler) {
    local_ai_handler_ = handler;
  }

 private:
  SessionPreferences();

  // The user data key.
  static int kLocatorKey;

  std::vector<PreloadScript> preload_scripts_;
  base::WeakPtr<api::UtilityProcessWrapper> local_ai_handler_;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_SESSION_PREFERENCES_H_
