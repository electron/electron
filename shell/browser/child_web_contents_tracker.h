// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_CHILD_WEB_CONTENTS_TRACKER_H_
#define ELECTRON_SHELL_BROWSER_CHILD_WEB_CONTENTS_TRACKER_H_

#include <string>

#include "content/public/browser/web_contents_user_data.h"

namespace electron {

// ChildWebContentsTracker tracks child WebContents
// created by native `window.open()`
struct ChildWebContentsTracker
    : public content::WebContentsUserData<ChildWebContentsTracker> {
  ~ChildWebContentsTracker() override;

  // disable copy
  ChildWebContentsTracker(const ChildWebContentsTracker&) = delete;
  ChildWebContentsTracker& operator=(const ChildWebContentsTracker&) = delete;

  GURL url;
  std::string frame_name;
  content::Referrer referrer;
  std::string raw_features;
  scoped_refptr<network::ResourceRequestBody> body;

 private:
  explicit ChildWebContentsTracker(content::WebContents* web_contents);
  friend class content::WebContentsUserData<ChildWebContentsTracker>;

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_CHILD_WEB_CONTENTS_TRACKER_H_
