#ifndef BRIGHTRAY_BROWSER_INSPECTABLE_WEB_CONTENTS_VIEW_MAC_H_
#define BRIGHTRAY_BROWSER_INSPECTABLE_WEB_CONTENTS_VIEW_MAC_H_

#import "browser/inspectable_web_contents_view.h"

#import "base/memory/scoped_nsobject.h"

@class BRYInspectableWebContentsView;

namespace brightray {

class InspectableWebContentsImpl;

class InspectableWebContentsViewMac : public InspectableWebContentsView {
public:
  InspectableWebContentsViewMac(InspectableWebContentsImpl*);
  
  virtual gfx::NativeView GetNativeView() const OVERRIDE;
  virtual void ShowDevTools() OVERRIDE;
  virtual void CloseDevTools() OVERRIDE;

  InspectableWebContentsImpl* inspectable_web_contents() { return inspectable_web_contents_; }

private:
  // Owns us.
  InspectableWebContentsImpl* inspectable_web_contents_;
  
  scoped_nsobject<BRYInspectableWebContentsView> view_;

  DISALLOW_COPY_AND_ASSIGN(InspectableWebContentsViewMac);
};

}

#endif
