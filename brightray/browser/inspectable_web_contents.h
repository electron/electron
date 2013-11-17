#ifndef BRIGHTRAY_INSPECTABLE_WEB_CONTENTS_H_
#define BRIGHTRAY_INSPECTABLE_WEB_CONTENTS_H_

#include "content/public/browser/web_contents.h"

namespace brightray {

class InspectableWebContentsView;

class InspectableWebContents {
public:
  static InspectableWebContents* Create(const content::WebContents::CreateParams&);

  // The returned InspectableWebContents takes ownership of the passed-in WebContents.
  static InspectableWebContents* Create(content::WebContents*);

  virtual ~InspectableWebContents() {}

  virtual InspectableWebContentsView* GetView() const = 0;
  virtual content::WebContents* GetWebContents() const = 0;

  virtual void ShowDevTools() = 0;
};

}  // namespace brightray

#endif
