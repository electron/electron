// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_RENDERER_PREFERENCES_HELPER_H_
#define ATOM_BROWSER_RENDERER_PREFERENCES_HELPER_H_

#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

namespace content {
class WebContents;
}

namespace atom {

class RendererPreferencesHelper :
              public content::WebContentsObserver,
              public content::WebContentsUserData<RendererPreferencesHelper> {
 public:
  ~RendererPreferencesHelper() override;

 private:
  explicit RendererPreferencesHelper(content::WebContents* contents);
  friend class content::WebContentsUserData<RendererPreferencesHelper>;

  // Update the WebContents's RendererPreferences.
  void UpdateRendererPreferences();
  DISALLOW_COPY_AND_ASSIGN(RendererPreferencesHelper);
};

}  // namespace atom

#endif  // ATOM_BROWSER_RENDERER_PREFERENCES_HELPER_H_
