#ifndef BRIGHTRAY_BROWSER_LINUX_INSPECTABLE_WEB_CONTENTS_VIEW_LINUX_H_
#define BRIGHTRAY_BROWSER_LINUX_INSPECTABLE_WEB_CONTENTS_VIEW_LINUX_H_

#include "browser/devtools_contents_resizing_strategy.h"
#include "browser/inspectable_web_contents_view.h"

#include "base/compiler_specific.h"

namespace brightray {

class InspectableWebContentsImpl;

class InspectableWebContentsViewLinux : public InspectableWebContentsView {
 public:
  explicit InspectableWebContentsViewLinux(
      InspectableWebContentsImpl* inspectable_web_contents_impl);
  ~InspectableWebContentsViewLinux();

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

  DevToolsContentsResizingStrategy strategy_;

  DISALLOW_COPY_AND_ASSIGN(InspectableWebContentsViewLinux);
};

}  // namespace brightray

#endif
