#ifndef ATOM_BROWSER_ROOT_WEB_CONTENTS_TRACKER_H_
#define ATOM_BROWSER_ROOT_WEB_CONTENTS_TRACKER_H_

#include "content/public/browser/web_contents_observer.h"

namespace atom {

// RootWebContentsTracker tracks root WebContents created by
// `new BrowserWindow`.
class RootWebContentsTracker : public content::WebContentsObserver {
 public:
  RootWebContentsTracker(content::WebContents* web_contents);
  static bool IsRootWebContents(content::WebContents* web_contents);

 protected:
  void WebContentsDestroyed() override;
};

}  // namespace atom

#endif  // ATOM_BROWSER_ROOT_WEB_CONTENTS_TRACKER_H_
