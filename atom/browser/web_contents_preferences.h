// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_WEB_CONTENTS_PREFERENCES_H_
#define ATOM_BROWSER_WEB_CONTENTS_PREFERENCES_H_

#include "base/values.h"
#include "content/public/browser/web_contents_user_data.h"

namespace atom {

// Stores and applies the preferences of WebContents.
class WebContentsPreferences
    : public content::WebContentsUserData<WebContentsPreferences> {
 public:
  // Get the preferences of |web_contents|.
  static WebContentsPreferences* From(content::WebContents* web_contents);

  WebContentsPreferences(content::WebContents* web_contents,
                         base::DictionaryValue&& web_preferences);
  ~WebContentsPreferences() override;

 private:
  friend class content::WebContentsUserData<WebContentsPreferences>;

  base::DictionaryValue web_preferences_;

  DISALLOW_COPY_AND_ASSIGN(WebContentsPreferences);
};

}  // namespace atom

#endif  // ATOM_BROWSER_WEB_CONTENTS_PREFERENCES_H_
