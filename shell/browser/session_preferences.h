// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_SESSION_PREFERENCES_H_
#define SHELL_BROWSER_SESSION_PREFERENCES_H_

#include <vector>

#include "base/files/file_path.h"
#include "base/supports_user_data.h"
#include "content/public/browser/browser_context.h"

namespace electron {

class SessionPreferences : public base::SupportsUserData::Data {
 public:
  static SessionPreferences* FromBrowserContext(
      content::BrowserContext* context);
  static std::vector<base::FilePath> GetValidPreloads(
      content::BrowserContext* context);

  explicit SessionPreferences(content::BrowserContext* context);
  ~SessionPreferences() override;

  void set_preloads(const std::vector<base::FilePath>& preloads) {
    preloads_ = preloads;
  }
  const std::vector<base::FilePath>& preloads() const { return preloads_; }

 private:
  // The user data key.
  static int kLocatorKey;

  std::vector<base::FilePath> preloads_;
};

}  // namespace electron

#endif  // SHELL_BROWSER_SESSION_PREFERENCES_H_
