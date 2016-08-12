// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef BRAVE_BROWSER_RENDERER_PREFERENCES_HELPER_H_
#define BRAVE_BROWSER_RENDERER_PREFERENCES_HELPER_H_

#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

namespace content {
class WebContents;
class RenderViewHost;
}

namespace brave {

class RendererPreferencesHelper :
              public content::WebContentsObserver,
              public content::WebContentsUserData<RendererPreferencesHelper> {
 public:
  ~RendererPreferencesHelper() override;

 private:
  explicit RendererPreferencesHelper(content::WebContents* contents);
  friend class content::WebContentsUserData<RendererPreferencesHelper>;

  // WebContentsObserver implementation
  virtual void RenderViewHostChanged(content::RenderViewHost* old_host,
                                     content::RenderViewHost* new_host);

  // Update the WebContents's RendererPreferences.
  void UpdateRendererPreferences();
  DISALLOW_COPY_AND_ASSIGN(RendererPreferencesHelper);
};

}  // namespace brave

#endif  // BRAVE_BROWSER_RENDERER_PREFERENCES_HELPER_H_
