// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_CHILD_WEB_CONTENTS_TRACKER_H_
#define ATOM_BROWSER_CHILD_WEB_CONTENTS_TRACKER_H_

#include "content/public/browser/web_contents_observer.h"

namespace atom {

// ChildWebContentsTracker tracks child WebContents
// created by native `window.open()`
class ChildWebContentsTracker : public content::WebContentsObserver {
 public:
  explicit ChildWebContentsTracker(content::WebContents* web_contents);
  static bool IsChildWebContents(content::WebContents* web_contents);

 protected:
  void WebContentsDestroyed() override;
};

}  // namespace atom

#endif  // ATOM_BROWSER_CHILD_WEB_CONTENTS_TRACKER_H_
