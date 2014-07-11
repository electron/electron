#ifndef BRIGHTRAY_BROWSER_INSPECTABLE_WEB_CONTENTS_VIEW_MAC_H_
#define BRIGHTRAY_BROWSER_INSPECTABLE_WEB_CONTENTS_VIEW_MAC_H_

#include "browser/inspectable_web_contents_view.h"

#include "base/mac/scoped_nsobject.h"

@class BRYInspectableWebContentsView;

namespace brightray {

class InspectableWebContentsImpl;

class InspectableWebContentsViewMac : public InspectableWebContentsView {
 public:
  explicit InspectableWebContentsViewMac(
      InspectableWebContentsImpl* inspectable_web_contents_impl);
  virtual ~InspectableWebContentsViewMac();

  virtual gfx::NativeView GetNativeView() const OVERRIDE;
  virtual void ShowDevTools() OVERRIDE;
  virtual void CloseDevTools() OVERRIDE;
  virtual bool IsDevToolsViewShowing() OVERRIDE;
  virtual void SetIsDocked(bool docked) OVERRIDE;
  virtual void SetContentsResizingStrategy(
      const DevToolsContentsResizingStrategy& strategy) OVERRIDE;

  InspectableWebContentsImpl* inspectable_web_contents() {
    return inspectable_web_contents_;
  }

 private:
  // Owns us.
  InspectableWebContentsImpl* inspectable_web_contents_;

  base::scoped_nsobject<BRYInspectableWebContentsView> view_;

  DISALLOW_COPY_AND_ASSIGN(InspectableWebContentsViewMac);
};

}  // namespace brightray

#endif
