#ifndef BRIGHTRAY_BROWSER_DEFAULT_WEB_CONTENTS_DELEGATE_H_
#define BRIGHTRAY_BROWSER_DEFAULT_WEB_CONTENTS_DELEGATE_H_

#include "content/public/browser/web_contents_delegate.h"

namespace brightray {

// This class provides some sane default behaviors to any content::WebContents
// instance (e.g., keyboard shortcut handling on Mac).
class DefaultWebContentsDelegate : public content::WebContentsDelegate {
 public:
  DefaultWebContentsDelegate();
  ~DefaultWebContentsDelegate();

 protected:
  bool CheckMediaAccessPermission(content::WebContents* web_contents,
                                  const GURL& security_origin,
                                  content::MediaStreamType type) override;
  void RequestMediaAccessPermission(
      content::WebContents*,
      const content::MediaStreamRequest&,
      const content::MediaResponseCallback&) override;
#if defined(OS_MACOSX)
  void HandleKeyboardEvent(
      content::WebContents*, const content::NativeWebKeyboardEvent&) override;
#endif
};

}  // namespace brightray

#endif
