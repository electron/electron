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

  void SetEnableBlinkFeatures(const std::string& features);
  void SetDisableBlinkFeatures(const std::string& features);
  const std::optional<std::string>& GetEnableBlinkFeatures() const;
  const std::optional<std::string>& GetDisableBlinkFeatures() const;

 private:
  SessionPreferences();

  // The user data key.
  static int kLocatorKey;

  std::vector<PreloadScript> preload_scripts_;
  std::optional<std::string> enable_blink_features_;
  std::optional<std::string> disable_blink_features_;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_SESSION_PREFERENCES_H_
