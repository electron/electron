// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_NET_PRECONNECT_MANAGER_TAB_HELPER_H_
#define ATOM_BROWSER_NET_PRECONNECT_MANAGER_TAB_HELPER_H_

#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

namespace predictors {
class PreconnectManager;
}

namespace atom {

class WebContentsPreferences;

class PreconnectManagerHelper
    : public content::WebContentsObserver,
      public content::WebContentsUserData<PreconnectManagerHelper> {
 public:
  ~PreconnectManagerHelper() override;

  // content::WebContentsObserver implementation
  void DidStartNavigation(
      content::NavigationHandle* navigation_handle) override;

  // -1 - in case of number of preconnect sockets option is not set
  static int GetNumberOfSocketsToPreconnect(WebContentsPreferences* prefs);

  void SetNumberOfSocketsToPreconnect(int numSockets);

 private:
  explicit PreconnectManagerHelper(content::WebContents* web_contents);
  friend class content::WebContentsUserData<PreconnectManagerHelper>;

  predictors::PreconnectManager* preconnect_manager_;

  int number_of_sockets_to_preconnect_;

  WEB_CONTENTS_USER_DATA_KEY_DECL();

  DISALLOW_COPY_AND_ASSIGN(PreconnectManagerHelper);
};

}  // namespace atom

#endif  // ATOM_BROWSER_NET_PRECONNECT_MANAGER_TAB_HELPER_H_
