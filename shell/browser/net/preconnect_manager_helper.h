// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_NET_PRECONNECT_MANAGER_HELPER_H_
#define SHELL_BROWSER_NET_PRECONNECT_MANAGER_HELPER_H_

#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

namespace predictors {
class PreconnectManager;
}

namespace electron {

class WebContentsPreferences;

namespace api {
class Session;
}

class PreconnectManagerHelper
    : public content::WebContentsObserver,
      public content::WebContentsUserData<PreconnectManagerHelper> {
 public:
  ~PreconnectManagerHelper() override;

  // content::WebContentsObserver implementation
  void DidStartNavigation(
      content::NavigationHandle* navigation_handle) override;

  void SetSession(api::Session* session);

 private:
  explicit PreconnectManagerHelper(content::WebContents* web_contents);
  friend class content::WebContentsUserData<PreconnectManagerHelper>;

  api::Session* session_;

  WEB_CONTENTS_USER_DATA_KEY_DECL();

  DISALLOW_COPY_AND_ASSIGN(PreconnectManagerHelper);
};

}  // namespace electron

#endif  // SHELL_BROWSER_NET_PRECONNECT_MANAGER_HELPER_H_
