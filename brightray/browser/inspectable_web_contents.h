#ifndef BRIGHTRAY_INSPECTABLE_WEB_CONTENTS_H_
#define BRIGHTRAY_INSPECTABLE_WEB_CONTENTS_H_

#include "content/public/browser/web_contents.h"

namespace brightray {

class InspectableWebContentsDelegate;
class InspectableWebContentsView;

class InspectableWebContents {
 public:
  static InspectableWebContents* Create(
      const content::WebContents::CreateParams&);

  // The returned InspectableWebContents takes ownership of the passed-in
  // WebContents.
  static InspectableWebContents* Create(content::WebContents*);

  virtual ~InspectableWebContents() {}

  virtual InspectableWebContentsView* GetView() const = 0;
  virtual content::WebContents* GetWebContents() const = 0;

  virtual void SetCanDock(bool can_dock) = 0;
  virtual void ShowDevTools() = 0;
  // Close the DevTools completely instead of just hide it.
  virtual void CloseDevTools() = 0;
  virtual bool IsDevToolsViewShowing() = 0;

  // The delegate manages its own life.
  virtual void SetDelegate(InspectableWebContentsDelegate* delegate) = 0;
  virtual InspectableWebContentsDelegate* GetDelegate() const = 0;
};

}  // namespace brightray

#endif
