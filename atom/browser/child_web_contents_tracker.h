#ifndef ATOM_BROWSER_CHILD_WEB_CONTENTS_TRACKER_H_
#define ATOM_BROWSER_CHILD_WEB_CONTENTS_TRACKER_H_

#include "content/public/browser/web_contents_observer.h"

namespace atom {

// ChildWebContentsTracker tracks root WebContents created by
// `new BrowserWindow`.
class ChildWebContentsTracker : public content::WebContentsObserver {
 public:
  ChildWebContentsTracker(content::WebContents* web_contents);
  static bool IsChildWebContents(content::WebContents* web_contents);

 protected:
  void WebContentsDestroyed() override;
};

}  // namespace atom

#endif  // ATOM_BROWSER_CHILD_WEB_CONTENTS_TRACKER_H_
