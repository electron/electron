#ifndef BRIGHTRAY_BROWSER_INSPECTABLE_WEB_CONTENTS_IMPL_H_
#define BRIGHTRAY_BROWSER_INSPECTABLE_WEB_CONTENTS_IMPL_H_

#include "browser/inspectable_web_contents.h"

namespace brightray {

class InspectableWebContentsView;

class InspectableWebContentsImpl : public InspectableWebContents {
public:
  InspectableWebContentsImpl(const content::WebContents::CreateParams&);
  virtual ~InspectableWebContentsImpl() OVERRIDE;

  virtual InspectableWebContentsView* GetView() const OVERRIDE;
  virtual content::WebContents* GetWebContents() const OVERRIDE;

  virtual void ShowDevTools() OVERRIDE;

  content::WebContents* devtools_web_contents() { return devtools_web_contents_.get(); }

private:
  scoped_ptr<content::WebContents> web_contents_;
  scoped_ptr<content::WebContents> devtools_web_contents_;
  scoped_ptr<InspectableWebContentsView> view_;

  DISALLOW_COPY_AND_ASSIGN(InspectableWebContentsImpl);
};

}

#endif
